#include <memory>
#include <iostream>

#include <io_factory.h>

#include <job_ansi_attributes.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/rectangle.h>

using namespace job::io;

using namespace job::ansi;

using namespace job::tui;
using namespace job::tui::gui;

int main() {
    auto device = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!device || !device->openDevice()) {
        std::cerr << "Failed to open IODevice\n";
        return 1;
    }

    auto app = std::make_shared<JobTuiCoreApplication>(device);

    auto root = Window::create({0, 0, 80, 40}, "Focus Test - Press Tab", app.get());

    auto item1 = Rectangle::create({3, 2, 20, 5}, "bright-magenta", root.get());
    item1->setOpacity(item1->focus() ? 255: 100);
    item1->onFocusChanged = [app, root, item1](bool event) {
        if (event) {
            root->setTitle("FOCUS 1 TRUE");
            item1->setOpacity(254);
        }else{
            item1->setOpacity(128);;
        }
        app->markDirty();
    };

    auto item2 = Rectangle::create({25, 2, 20, 5}, "bright-green", root.get());
    item2->onFocusChanged = [app, root, item2](bool focus) {
        if (focus) {
            root->setTitle("FOCUS 2 TRUE");
            item2->setOpacity(254);
        }else {
            item2->setOpacity(128);
        }
        // item2->
        app->markDirty();
    };

    return app->exec();
}
