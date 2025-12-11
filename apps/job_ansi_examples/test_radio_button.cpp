#include <memory>
#include <iostream>

#include <io_factory.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <layout/linear_layout.h>
#include <elements/label.h>
#include <elements/radio_button.h>

using namespace job::io;
using namespace job::tui;
using namespace job::tui::gui;

int main() {
    auto dev = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!dev || !dev->openDevice()) {
        std::cerr << "Failed to open IODevice\n";
        return 1;
    }

    // window in screen-space
    constexpr int WIN_X = 0;
    constexpr int WIN_Y = 0;
    constexpr int WIN_W = 80;
    constexpr int WIN_H = 24;

    auto app  = std::make_shared<JobTuiCoreApplication>(dev);
    auto root = Window::create({WIN_X, WIN_Y, WIN_W, WIN_H},
                               "Radio Demo (Tab: focus • Arrows: move • Space: select)",
                               app.get());

    // --- content area in screen-space (pad 2 on all sides so we never touch the frame)
    constexpr int PAD   = 2;
    const int CONTENT_X = WIN_X + PAD;
    const int CONTENT_Y = WIN_Y + PAD;
    const int CONTENT_W = WIN_W - PAD*2;
    const int CONTENT_H = WIN_H - PAD*2;

    // two columns in screen-space
    constexpr int GUTTER = 4;                  // visual gap between columns
    const int COL_W = (CONTENT_W - GUTTER) / 2;

    // Group A column
    auto groupA = JobTuiItem::create({CONTENT_X, CONTENT_Y, COL_W, CONTENT_H}, root.get());
    groupA->setItemHasContents(true);

    // layout inside groupA will place children at group's ABSOLUTE x,y
    auto colA = LinearLayout::create(groupA->boundingRect(), groupA.get());
    colA->setOrientation(LayoutEngine::Orientation::Vertical);
    colA->setSpacing(0);

    Label::create({0,0,COL_W,1}, "Group A", colA.get());
    auto rA1 = RadioButton::create({0,0,COL_W,1}, "Apple",  colA.get());  rA1->setGroupName("fruits");
    auto rA2 = RadioButton::create({0,0,COL_W,1}, "Banana", colA.get());  rA2->setGroupName("fruits");
    auto rA3 = RadioButton::create({0,0,COL_W,1}, "Cherry", colA.get());  rA3->setGroupName("fruits");
    rA2->setChecked(true);

    // Group B column
    auto groupB = JobTuiItem::create({CONTENT_X + COL_W + GUTTER, CONTENT_Y, COL_W, CONTENT_H}, root.get());
    groupB->setItemHasContents(true);

    auto colB = LinearLayout::create(groupB->boundingRect(), groupB.get());
    colB->setOrientation(LayoutEngine::Orientation::Vertical);
    colB->setSpacing(0);

    Label::create({0,0,COL_W,1}, "Group B", colB.get());
    auto rB1 = RadioButton::create({0,0,COL_W,1}, "Cat", colB.get());  rB1->setGroupName("animals");
    auto rB2 = RadioButton::create({0,0,COL_W,1}, "Dog", colB.get());  rB2->setGroupName("animals");
    auto rB3 = RadioButton::create({0,0,COL_W,1}, "Fox", colB.get());  rB3->setGroupName("animals");
    rB2->setChecked(true);

    // lay out both columns (these compute ABSOLUTE child rects)
    colA->applyLayout();
    colB->applyLayout();

    // title update on focus
    auto titleOnFocus = [root](const std::string& g, const std::string& t){
        root->setTitle("Radio Demo — focus: [" + g + "] " + t + " (Tab/Arrows/Space)");
    };
    rA1->onFocusChanged = [=](bool f){ if (f) titleOnFocus("fruits","Apple");  };
    rA2->onFocusChanged = [=](bool f){ if (f) titleOnFocus("fruits","Banana"); };
    rA3->onFocusChanged = [=](bool f){ if (f) titleOnFocus("fruits","Cherry"); };
    rB1->onFocusChanged = [=](bool f){ if (f) titleOnFocus("animals","Cat");    };
    rB2->onFocusChanged = [=](bool f){ if (f) titleOnFocus("animals","Dog");    };
    rB3->onFocusChanged = [=](bool f){ if (f) titleOnFocus("animals","Fox");    };

    return app->exec();
}
