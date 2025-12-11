#include <memory>
#include <iostream>

#include <io_factory.h>

#include <job_tui_core_application.h>
#include <containers/window.h>

using namespace job::io;
using namespace job::tui;
using namespace job::tui::gui;

int main()
{
    auto device = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!device || !device->openDevice()) {
        std::cerr << "Failed to open IODevice\n";
        return 1;
    }
    auto app = std::make_shared<JobTuiCoreApplication>(device);

    auto root = Window::create(
        {2, 1, 50, 10},                 // Bounding Rect
        "Main Window",                  // Title string
        Window::TitleAlignment::Left,   // Title alignment
        app.get()                       // parent
        );

    return app->exec();
}
