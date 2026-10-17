/*
 * Copyright (C) 2026 LingmoOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "x11windows.h"

#include <QGuiApplication>
#include <QByteArray>
#include <QVector>

#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xtest.h>

#include <cstdlib>
#include <cstring>

namespace {

xcb_connection_t *connection()
{
    auto *x11App = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    return x11App ? x11App->connection() : nullptr;
}

xcb_window_t rootWindow(xcb_connection_t *c)
{
    return xcb_setup_roots_iterator(xcb_get_setup(c)).data->root;
}

xcb_atom_t atom(xcb_connection_t *c, const char *name)
{
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(c, false, strlen(name), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(c, cookie, nullptr);
    xcb_atom_t result = reply ? reply->atom : XCB_ATOM_NONE;
    free(reply);
    return result;
}

template <typename T>
QVector<T> property(xcb_connection_t *c, xcb_window_t window, xcb_atom_t prop, xcb_atom_t type)
{
    QVector<T> result;

    if (prop == XCB_ATOM_NONE)
        return result;

    xcb_get_property_cookie_t cookie = xcb_get_property(c, false, window, prop, type, 0, 4096);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, nullptr);

    if (reply && reply->type == type && reply->format == 32) {
        const T *data = static_cast<const T *>(xcb_get_property_value(reply));
        const int len = xcb_get_property_value_length(reply) / 4;
        for (int i = 0; i < len; ++i)
            result.append(data[i]);
    }

    free(reply);
    return result;
}


QString windowTitle(xcb_connection_t *c, xcb_window_t window)
{
    const xcb_atom_t netName = atom(c, "_NET_WM_NAME");
    const xcb_atom_t utf8 = atom(c, "UTF8_STRING");

    for (const auto &[prop, type] : { std::pair { netName, utf8 }, std::pair { xcb_atom_t(XCB_ATOM_WM_NAME), xcb_atom_t(XCB_ATOM_STRING) } }) {
        if (prop == XCB_ATOM_NONE)
            continue;
        xcb_get_property_reply_t *reply =
                xcb_get_property_reply(c, xcb_get_property(c, false, window, prop, type, 0, 1024), nullptr);
        if (!reply)
            continue;
        const int len = xcb_get_property_value_length(reply);
        QString title;
        if (reply->type == type && reply->format == 8 && len > 0)
            title = QString::fromUtf8(static_cast<const char *>(xcb_get_property_value(reply)), len);
        free(reply);
        if (!title.isEmpty())
            return title;
    }
    return QString();
}

// Frame geometry of a managed client window.
bool frameGeometry(xcb_connection_t *c, xcb_window_t window, bool managed, QRect *result)
{
    const xcb_window_t root = rootWindow(c);

    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(c, xcb_get_geometry(c, window), nullptr);
    if (!geo)
        return false;

    xcb_translate_coordinates_reply_t *pos =
            xcb_translate_coordinates_reply(c, xcb_translate_coordinates(c, window, root, 0, 0), nullptr);
    if (!pos) {
        free(geo);
        return false;
    }

    QRect rect(pos->dst_x, pos->dst_y, geo->width, geo->height);
    if (!managed)
        rect.adjust(-geo->border_width, -geo->border_width, geo->border_width, geo->border_width);
    free(geo);
    free(pos);

    if (managed) {
        // left, right, top, bottom
        const QVector<quint32> frame = property<quint32>(c, window, atom(c, "_NET_FRAME_EXTENTS"), XCB_ATOM_CARDINAL);
        if (frame.size() == 4)
            rect.adjust(-int(frame[0]), -int(frame[2]), int(frame[1]), int(frame[3]));

        // Client side decorations: remove the invisible shadow area.
        const QVector<quint32> gtkFrame = property<quint32>(c, window, atom(c, "_GTK_FRAME_EXTENTS"), XCB_ATOM_CARDINAL);
        if (gtkFrame.size() == 4)
            rect.adjust(int(gtkFrame[0]), int(gtkFrame[2]), -int(gtkFrame[1]), -int(gtkFrame[3]));
    }

    *result = rect;
    return rect.width() > 1 && rect.height() > 1;
}

}

QList<QRect> X11Windows::visibleWindows()
{
    QList<QRect> rects;
    for (const Window &w : windows())
        rects.append(w.rect);
    return rects;
}

bool X11Windows::activeWindow(Window *window)
{
    xcb_connection_t *c = connection();
    if (!c)
        return false;

    const QVector<xcb_window_t> active =
            property<xcb_window_t>(c, rootWindow(c), atom(c, "_NET_ACTIVE_WINDOW"), XCB_ATOM_WINDOW);
    if (active.isEmpty() || active.first() == XCB_WINDOW_NONE)
        return false;

    Window w;
    w.id = active.first();
    if (!frameGeometry(c, w.id, true, &w.rect))
        return false;
    w.title = windowTitle(c, w.id);

    if (window)
        *window = w;
    return true;
}

bool X11Windows::scroll(const QPoint &pos, int clicks)
{
    xcb_connection_t *c = connection();
    if (!c)
        return false;

    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(c, &xcb_test_id);
    if (!ext || !ext->present)
        return false;

    const xcb_window_t root = rootWindow(c);
    xcb_test_fake_input(c, XCB_MOTION_NOTIFY, 0, XCB_CURRENT_TIME, root, pos.x(), pos.y(), 0);

    // Button 4 scrolls up, 5 down.
    const uint8_t button = clicks > 0 ? 5 : 4;
    for (int i = 0; i < qAbs(clicks); ++i) {
        xcb_test_fake_input(c, XCB_BUTTON_PRESS, button, XCB_CURRENT_TIME, root, 0, 0, 0);
        xcb_test_fake_input(c, XCB_BUTTON_RELEASE, button, XCB_CURRENT_TIME, root, 0, 0, 0);
    }
    xcb_flush(c);
    return true;
}

QList<X11Windows::Window> X11Windows::windows()
{
    QList<Window> windows;
    xcb_connection_t *c = connection();

    if (!c)
        return windows;

    const xcb_window_t root = rootWindow(c);
    const xcb_atom_t clientListAtom = atom(c, "_NET_CLIENT_LIST_STACKING");
    const xcb_atom_t stateAtom = atom(c, "_NET_WM_STATE");
    const xcb_atom_t hiddenAtom = atom(c, "_NET_WM_STATE_HIDDEN");
    const xcb_atom_t typeAtom = atom(c, "_NET_WM_WINDOW_TYPE");
    const xcb_atom_t desktopTypeAtom = atom(c, "_NET_WM_WINDOW_TYPE_DESKTOP");
    const xcb_atom_t dockTypeAtom = atom(c, "_NET_WM_WINDOW_TYPE_DOCK");
    const xcb_atom_t currentDesktopAtom = atom(c, "_NET_CURRENT_DESKTOP");
    const xcb_atom_t wmDesktopAtom = atom(c, "_NET_WM_DESKTOP");

    // Bottom to top.
    QVector<xcb_window_t> stack = property<xcb_window_t>(c, root, clientListAtom, XCB_ATOM_WINDOW);
    const bool managed = !stack.isEmpty();

    if (!managed) {
        // No window manager: fall back to the children of the root window.
        xcb_query_tree_reply_t *tree = xcb_query_tree_reply(c, xcb_query_tree(c, root), nullptr);
        if (tree) {
            xcb_window_t *children = xcb_query_tree_children(tree);
            for (int i = 0; i < xcb_query_tree_children_length(tree); ++i)
                stack.append(children[i]);
            free(tree);
        }
    }

    const QVector<quint32> currentDesktop = property<quint32>(c, root, currentDesktopAtom, XCB_ATOM_CARDINAL);

    for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
        const xcb_window_t window = *it;

        xcb_get_window_attributes_reply_t *attrs =
                xcb_get_window_attributes_reply(c, xcb_get_window_attributes(c, window), nullptr);
        if (!attrs)
            continue;

        const bool viewable = attrs->map_state == XCB_MAP_STATE_VIEWABLE;
        const bool overrideRedirect = attrs->override_redirect;
        free(attrs);

        if (!viewable || (!managed && overrideRedirect))
            continue;

        if (managed) {
            if (property<xcb_atom_t>(c, window, stateAtom, XCB_ATOM_ATOM).contains(hiddenAtom))
                continue;

            const QVector<xcb_atom_t> types = property<xcb_atom_t>(c, window, typeAtom, XCB_ATOM_ATOM);
            if (types.contains(desktopTypeAtom) || types.contains(dockTypeAtom))
                continue;

            const QVector<quint32> desktop = property<quint32>(c, window, wmDesktopAtom, XCB_ATOM_CARDINAL);
            if (!desktop.isEmpty() && !currentDesktop.isEmpty()
                    && desktop.first() != 0xFFFFFFFF && desktop.first() != currentDesktop.first())
                continue;
        }

        Window w;
        w.id = window;
        if (!frameGeometry(c, window, managed, &w.rect))
            continue;
        w.title = windowTitle(c, window);
        windows.append(w);
    }

    return windows;
}

bool X11Windows::pointerPosition(QPoint *pos)
{
    xcb_connection_t *c = connection();

    if (!c)
        return false;

    xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(c, xcb_query_pointer(c, rootWindow(c)), nullptr);
    if (!reply)
        return false;

    if (pos)
        *pos = QPoint(reply->root_x, reply->root_y);

    free(reply);
    return true;
}

bool X11Windows::selectRawButtonPress(bool enable)
{
    xcb_connection_t *c = connection();

    if (!c)
        return false;

    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(c, &xcb_input_id);
    if (!ext || !ext->present)
        return false;

    struct {
        xcb_input_event_mask_t head;
        uint32_t mask;
    } mask;

    mask.head.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
    mask.head.mask_len = 1;
    mask.mask = enable ? XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_PRESS : 0;

    xcb_input_xi_select_events(c, rootWindow(c), 1, &mask.head);
    xcb_flush(c);

    return true;
}

bool X11Windows::isRawButtonPress(void *message)
{
    xcb_connection_t *c = connection();
    auto *event = static_cast<xcb_generic_event_t *>(message);

    if (!c || (event->response_type & ~0x80) != XCB_GE_GENERIC)
        return false;

    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(c, &xcb_input_id);
    auto *ge = reinterpret_cast<xcb_ge_generic_event_t *>(event);

    if (!ext || !ext->present || ge->extension != ext->major_opcode
            || ge->event_type != XCB_INPUT_RAW_BUTTON_PRESS)
        return false;

    // Ignore the scroll wheel.
    auto *press = reinterpret_cast<xcb_input_raw_button_press_event_t *>(event);
    return press->detail >= 1 && press->detail <= 3;
}
