#include <memory>
#include <iostream>

#include <io_factory.h>
#include <base/ansipp_core_application.h>
#include <gui/containers/window.h>
#include <core/attributes.h>

using namespace ansipp;
using namespace ansipp::tui::gui;
using namespace ansipp::tui::base;

int main() {
    auto device = io::IOFactory::createFromType(io::FactoryType::FILE_STD_OUT, "");
    if (!device || !device->openDevice()) {
        std::cerr << "Failed to open IODevice\n";
        return 1;
    }

    auto app = std::make_shared<AnsippCoreApplication>(device);

    auto root = std::make_shared<Window>();
    root->setTitle("Main Window");
    root->setGeometry(2, 1, 50, 10);

    auto item = std::make_shared<AnsippItem>(root.get());
    item->setGeometry(3, 2, 20, 5);
    auto itemAtt = std::make_shared<core::Attributes>();
    itemAtt->setBackground(utils::RGBColor(0, 128, 255));
    item->setAttributes(itemAtt);
    root->addChild(item);

    auto item2 = std::make_shared<AnsippItem>(root.get());
    item2->setGeometry(25, 2, 20, 5);
    auto item2Attr = std::make_shared<core::Attributes>();
    item2Attr->setBackground(utils::RGBColor(0, 255, 128));
    item2->setAttributes(item2Attr);
    root->addChild(item2);

    app->setRoot(root);
    app->start();

    return app->exec();
}
