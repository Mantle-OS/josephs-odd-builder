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

    root->setTitle("Base Window");
    root->setGeometry(2, 1, 50, 10);

    app->setRoot(root);
    app->start();

    return app->exec();
}
