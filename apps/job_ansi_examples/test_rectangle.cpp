#include <memory>
#include <iostream>

#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/rectangle.h>

using namespace job::io;
using namespace job::ansi::utils::colors;
using namespace job::tui;
using namespace job::tui::gui;

int main() {
    auto device = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!device || !device->openDevice()) {
        std::cerr << "Failed to open IODevice\n";
        return 1;
    }

    auto app = std::make_shared<JobTuiCoreApplication>(device);
    auto root = Window::create({0, 0, 80, 40}, "Rectangle Example", app.get());

    auto rect = Rectangle::create({5, 3, 20, 10}, root.get());
    rect->setColor(Yellow());
    rect->setHasBorder(true);
    rect->setBorderColor(Blue());
    rect->setBorderStyle(DrawContext::BorderStyle::LightShade);

    return app->exec();
}
