#include <memory>
#include <iostream>

#include <io_factory.h>
#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/rectangle.h>

using namespace job::io;
using namespace job::tui;
using namespace job::tui::gui;

int main() {
    auto device = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!device || !device->openDevice()) {
        std::cerr << "Failed to open IODevice\n";
        return 1;
    }

    auto app = std::make_shared<JobTuiCoreApplication>(device);
    auto root = Window::create({0, 0, 80, 40}, "Anchor Example", app.get());

    // Rectangle that fills the window
    auto fillRect = Rectangle::create({0, 0, 78, 20}, "cyan", root.get());
    fillRect->anchors().setMargins(4);
    fillRect->anchors().fill = root.get();
    fillRect->resolveAnchors();

    // Centered rectangle
    auto centerRect = Rectangle::create({0, 0, 20, 6}, "green", fillRect.get());
    centerRect->anchors().centerIn = fillRect.get();
    centerRect->resolveAnchors();

    // Top-left anchored rectangle with margin
    auto topLeftRect = Rectangle::create({0, 0, 10, 3}, "magenta", fillRect.get());
    topLeftRect->anchors().top = fillRect.get();
    topLeftRect->anchors().left = fillRect.get();
    topLeftRect->anchors().topMargin = 1;
    topLeftRect->anchors().leftMargin = 1;
    topLeftRect->resolveAnchors();

    return app->exec();
}

