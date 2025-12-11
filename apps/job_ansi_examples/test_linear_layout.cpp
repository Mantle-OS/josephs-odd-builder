#include <memory>
#include <iostream>

#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <layout/linear_layout.h>
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
    auto root = Window::create({0, 0, 80, 40}, "LinearLayout Example", app.get());

    auto item = JobTuiItem::create({2, 2, 50, 20}, root.get());

    item->setItemHasContents(true);
    auto layout = LinearLayout::create(item->boundingRect(), item.get());
    layout->setOrientation(LayoutEngine::Orientation::Vertical);
    layout->setSpacing(1);

    for (int i = 0; i < 3; ++i) {
        auto rect = Rectangle::create({0, 0, 30, 3}, layout.get());
        switch (i) {
        case 0:
            rect->setColor(Red());
            break;
        case 1:
            rect->setColor(Green());
            break;
        case 2:
            rect->setColor(Blue());
            break;
        }
    }

    // Apply layout before starting the application
    item->applyLayout();

    return app->exec();
}
