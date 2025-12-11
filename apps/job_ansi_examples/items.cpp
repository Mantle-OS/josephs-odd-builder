#include <memory>
#include <iostream>

#include <io_factory.h>

#include <job_branch_and_bound.h>

#include <job_tui_core_application.h>
#include <containers/window.h>

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
    auto root = Window::create({2, 1, 80, 40}, "Main Window", app.get());

    // by default itemHasContents is false; we can set thsi or pass in the Attributes
    auto itemAtt = std::make_shared<Attributes>();
    itemAtt->setBackground(RGBColor(0, 128, 255));
    auto item = JobTuiItem::create({3, 2, 20, 5}, itemAtt, root.get());

    auto item2Attr = std::make_shared<Attributes>();
    item2Attr->setBackground(utils::RGBColor(0, 255, 128));
    auto item2 = JobTuiItem::create({25, 2, 20, 5}, item2Attr, root.get());

    return app->exec();
}
