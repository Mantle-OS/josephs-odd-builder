#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <containers/window.h>

#include <elements/button.h>

using namespace job::io;
using namespace job::ansi;
using namespace job::ansi::utils::colors;
using namespace job::tui;
using namespace job::tui::gui;

int main() {
    auto dev = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!dev || !dev->openDevice()) return 1;

    auto app  = std::make_shared<JobTuiCoreApplication>(dev);
    auto root = Window::create({0,0,60,12}, "Buttons", app.get());

    auto b1 = Button::create({2,2,14,3}, "Continue", root.get());
    auto b2 = Button::create({18,2,14,3}, "Cancel",   root.get());

    // Simple theming
    auto normal = std::make_shared<Attributes>();
    normal->setBackground(Blue()); normal->setForeground(White());
    auto focused = std::make_shared<Attributes>(*normal);
    focused->setBackground(BrightBlue());
    auto pressed = std::make_shared<Attributes>(*normal);
    pressed->setBackground(Cyan());
    auto disabled = std::make_shared<Attributes>();
    disabled->setBackground(Black()); disabled->setForeground(BrightBlack());

    for (auto btn : {b1, b2}) {
        btn->setNormalAttributes(normal);
        btn->setFocusedAttributes(focused);
        btn->setPressedAttributes(pressed);
        btn->setDisabledAttributes(disabled);
        btn->onClicked = [app, root, btn] {
            root->setTitle("Clicked: " + btn->text());
            app->markDirty();
        };
    }

    return app->exec();
}
