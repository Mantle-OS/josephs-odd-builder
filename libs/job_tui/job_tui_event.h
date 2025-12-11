#pragma once

#include <string>
#include <utility>

namespace job::tui {

class Event {
public:
    enum class EventType {
        None,
        KeyPress,
        KeyRelease,
        MousePress,
        MouseRelease,
        MouseMove,
        Timer,
        Custom,
        Escape,
        Quit,
        FocusIn,
        FocusOut,
        Unknown
    };

    enum class MouseButton {
        Left = 0,
        Middle = 1,
        Right = 2,
        Release = 3,
        ScrollUp = 64,
        ScrollDown = 65,
        Unknown = -1
    };

    Event(EventType type = EventType::None, int x = 0, int y = 0, int globalX = 0, int globalY = 0);

    EventType type() const;
    void setEventType(EventType eType);

    std::pair<int, int> pos() const;
    std::pair<int, int> globalPos() const;

    int x() const;
    int y() const;
    int globalX() const;
    int globalY() const;

    std::string key() const;
    void setKey(const std::string &newKey);

    MouseButton mouseButton() const;
    void setMouseButton(MouseButton btn);

    bool shiftModifier() const;
    bool ctrlModifier() const;
    bool altModifier() const;
    void setModifiers(bool shift, bool ctrl, bool alt);

    static Event fromRawInput(const char *data, std::size_t length);

private:
    EventType       m_type = EventType::None;
    int             m_x = 0;
    int             m_y = 0;
    int             m_globalX = 0;
    int             m_globalY = 0;
    std::string     m_key;
    MouseButton     m_mouseButton = MouseButton::Unknown;
    bool            m_shift = false;
    bool            m_ctrl = false;
    bool            m_alt = false;
};

}
