#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/checkbox.h>

using namespace job::io;
using namespace job::ansi;
using namespace job::ansi::utils::colors;
using namespace job::tui;
using namespace job::tui::gui;

int main() {
    auto dev = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!dev || !dev->openDevice())
        return 1;

    auto app  = std::make_shared<JobTuiCoreApplication>(dev);
    auto root = Window::create({0,0,60,12}, "Checkboxes", app.get());

    auto normal = std::make_shared<Attributes>();
    normal->setBackground(Black());
    normal->setForeground(White());

    auto focusd = std::make_shared<Attributes>(*normal);
    focusd->setBackground(Blue());

    auto disabled = std::make_shared<Attributes>();
    disabled->setBackground(Black());
    disabled->setForeground(BrightBlack());

    auto c1 = Checkbox::create({2,2,30,3}, "Install Mantle OS", root.get());
    c1->setNormalAttributes(normal);
    c1->setFocusedAttributes(focusd);
    c1->setDisabledAttributes(disabled);
    c1->onToggled = [root, app](bool on) {
        root->setTitle(std::string("Install Mantle OS: ") + (on ? "YES" : "NO"));
        app->markDirty();
    };

    auto c2 = Checkbox::create({2,5,30,3}, "Enable Networking", root.get());
    c2->setNormalAttributes(normal);
    c2->setFocusedAttributes(focusd);
    c2->onToggled = [root, app](bool on) {
        root->setTitle(std::string("Networking: ") + (on ? "ENABLED" : "DISABLED"));
        app->markDirty();
    };

    return app->exec();
}
