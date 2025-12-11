#include <memory>
#include <iostream>

#include <io_factory.h>

#include <color_utils.h>

#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/label.h>

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
    auto root = Window::create({0, 0, 80, 40}, "Label Showcase", app.get());

    // Label 1: NoWrap + Left alignment
    auto label1 = Label::create({2, 3, 56, 1},
                                "NoWrap, clipped: Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
                                root.get());
    label1->setWrapMode(Label::WrapMode::NoWrap);
    label1->setHorizontalAlignment(Label::TextAlignment::AlignLeft);

    // Label 2: CharWrap + Center alignment
    auto label2 = Label::create({2, 4, 56, 3},
                                "CharWrap: This line will break by character width.",
                                Label::WrapMode::CharWrap,
                                Label::TextAlignment::AlignHCenter,
                                Label::TextAlignment::AlignVCenter,
                                root.get());

    // Label 3: WordWrap + Right alignment(2 rows)
    auto label3 = Label::create({2, 5, 56, 4},
                                "WordWrap + AlignRight: The quick brown fox jumps over the lazy dog.",
                                Label::WrapMode::WordWrap,
                                Label::TextAlignment::AlignRight,
                                Label::TextAlignment::AlignVCenter,
                                root.get()
                                );


    // Label 4: WordWrap + Elide (force small box)
    auto label4 = Label::create({2, 7, 20, 2}, root.get());
    label4->setText("This is a very long sentence that must be wrapped or elided.");
    label4->setWrapMode(Label::WrapMode::NoWrap);
    label4->setElideMode(Label::ElideMode::ElideRight);


    // Colors and features
    // Label 5: Bold + Foreground color
    auto label5 = Label::create({2, 10, 40, 1}, root.get());
    label5->setText("Bold + Bright Cyan");
    label5->setBold(true); // Works
    label5->setForeground(utils::colors::BrightCyan());

    // Label 6: Italic + Underline
    auto label6 = Label::create({2, 11, 40, 1}, root.get());
    label6->setText("Italic + Underline (dotted)");
    label6->setItalic(true); // Broken
    label6->setUnderline(UnderlineStyle::Dotted);
    label6->setForeground(utils::colors::Yellow());

    // Label 7: Framed + Inverse
    auto label7 = Label::create({2, 12, 40, 1}, root.get());
    label7->setText("Framed + Inverse");
    label7->attributes()->framed = true;  // not sure ?
    label7->attributes()->inverse = true;

    // Label 8: Custom BG + Strikethrough
    auto label8 = Label::create({2, 13, 40, 1}, root.get());
    label8->setText("Red background + Strikethrough");
    label8->setBackground(utils::colors::Red());
    label8->setForeground(utils::colors::White());
    label8->attributes()->strikethrough = true;

    // Label 9: Encircled + Overline
    auto label9 = Label::create({2, 15, 40, 1}, root.get());
    label9->setText("Encircled + Overline");
    label9->setForeground(utils::colors::BrightGreen());
    label9->attributes()->encircled = true;
    label9->attributes()->overline = true; // Broken

    return app->exec();
}
