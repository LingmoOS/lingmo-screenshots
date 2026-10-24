#pragma once

class Application;

// Lingmo Intents on com.lingmo.Screenshot /Intents: screenshot.capture,
// screen.record.start / screen.record.stop and screenshot.ocr. Served by the
// running instance; `lingmo-screenshot --intents` (D-Bus activation) starts
// one without any window.
void publishScreenshotIntents(Application *app, bool headless);
