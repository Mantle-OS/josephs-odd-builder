#include <memory>
#include <iostream>

#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <job_tui_gradient.h>
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
    auto root = Window::create({0, 0, 80, 40}, "Gradient Rectangle", app.get());

    auto rect = Rectangle::create({2, 2, 77, 20}, root.get());
    auto grad = std::make_unique<Gradient>(GradientOrientation::VERTICAL);
    grad->addStop(0,   Blue());
    grad->addStop(100, Yellow());

    rect->setGradient(std::move(grad));

    return app->exec();
}
