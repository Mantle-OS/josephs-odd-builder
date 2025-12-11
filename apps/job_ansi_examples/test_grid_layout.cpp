#include <memory>
#include <iostream>

#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <layout/grid_layout.h>
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
    auto root = Window::create({0, 0, 80, 40}, "Grid Example", app.get());

    auto item = JobTuiItem::create({2, 3, 40, 20}, root.get());
    item->setItemHasContents(true);

    auto grid = GridLayout::create({2, 3, 40, 20}, 4, 1,
                                   {
                                       Rectangle::create({0, 0, 40, 3}, "red"),
                                       Rectangle::create({0, 0, 40, 3}, "green"),
                                       Rectangle::create({0, 0, 40, 3}, "blue"),
                                       Rectangle::create({0, 0, 40, 3}, "yellow"),
                                       Rectangle::create({0, 0, 40, 3}, "magenta")
                                   },
                                   item.get()
                                   );
    item->applyLayout();

    return app->exec();
}
