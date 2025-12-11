#include <memory>
#include <iostream>

#include <io_factory.h>

#include <job_ansi_attributes.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/rectangle.h>

#include <job_tui_mouse_area.h>

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

    auto root = Window::create({2, 1, 80, 40}, "Click Test - Use Mouse", app.get());

    // First rectangle
    auto rect1 = Rectangle::create(root.get());
    rect1->setGeometry(5, 3, 20, 10);
    rect1->setColor("blue");

    auto mouseOne = JobTuiMouseArea::create(rect1.get());
    mouseOne->onClicked = [rect1, root, app](const Event &) {
        rect1->color() == "green" ? rect1->setColor("red") : rect1->setColor("green");
        root->setTitle("Clicked Rect 1");
        app->markDirty();
    };

    // Second rectangle
    auto rect2 = Rectangle::create(root.get());
    rect2->setGeometry(30, 3, 20, 5);
    rect2->setColor("green");

    auto ms2 = JobTuiMouseArea::create(rect2.get());
    ms2->onClicked = [rect2, root, app](const Event &) {
        rect2->color() == "yellow" ? rect2->setColor("cyan") : rect2->setColor("yellow");
        root->setTitle("Clicked Rect 2");
        app->markDirty();
    };

    return app->exec();
}
