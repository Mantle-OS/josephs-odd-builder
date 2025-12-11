#include "job_tui_event.h"
#include <cstdlib>
#include <cstring>

#include <job_ansi_suffix.h>

namespace job::tui {

Event::Event(EventType type, int x, int y, int globalX, int globalY) :
    m_type(type),
    m_x(x),
    m_y(y),
    m_globalX(globalX),
    m_globalY(globalY)
{
}

void Event::setEventType(EventType eType)
{
    m_type = eType;
}

std::pair<int, int> Event::pos() const
{
    return {m_x, m_y};
}

std::pair<int, int> Event::globalPos() const
{
    return {m_globalX, m_globalY};
}

int Event::x() const
{
    return m_x;
}

int Event::y() const
{
    return m_y;
}

int Event::globalX() const
{
    return m_globalX;
}

int Event::globalY() const
{
    return m_globalY;
}

std::string Event::key() const
{
    return m_key;
}

void Event::setKey(const std::string &key)
{
    m_key = key;
}

Event::EventType Event::type() const
{
    return m_type;
}

Event::MouseButton Event::mouseButton() const
{
    return m_mouseButton;
}

void Event::setMouseButton(MouseButton btn)
{
    m_mouseButton = btn;
}

bool Event::shiftModifier() const
{
    return m_shift;
}

bool Event::ctrlModifier() const
{
    return m_ctrl;
}

bool Event::altModifier() const
{
    return m_alt;
}

void Event::setModifiers(bool shift, bool ctrl, bool alt)
{
    m_shift = shift;
    m_ctrl = ctrl;
    m_alt = alt;
}

Event Event::fromRawInput(const char *data, std::size_t length)
{
    Event event;

    if (length == 0 || !data) {
        event.setEventType(EventType::Unknown);
        return event;
    }

    if (data[0] == '\x1B') { // ESC sequence
        if (length >= 3 && data[1] == '[') {
            switch (data[2]) {
                case 'A':
                    event.setEventType(EventType::KeyPress);
                    event.setKey("Up");
                    return event;
                case 'B':
                    event.setEventType(EventType::KeyPress);
                    event.setKey("Down");
                    return event;
                case 'C':
                    event.setEventType(EventType::KeyPress);
                    event.setKey("Right");
                    return event;
                case 'D':
                    event.setEventType(EventType::KeyPress);
                    event.setKey("Left");
                    return event;
                default: break;
            }
        }

        // Mouse event stub (SGR mode: \x1b[<b;x;yM or m)
        if (length >= 6 && data[1] == '[' && data[2] == '<') {
            int button = 0, x = 0, y = 0;
            char m_type = data[length - 1];

            // Copy to temporary buffer for null-termination
            char buf[64] = {0};
            std::snprintf(buf, sizeof(buf), "%.*s", static_cast<int>(length), data);

            if (std::sscanf(buf, "\x1B[<%d;%d;%d%c", &button, &x, &y, &m_type) == 4) {
                // M
                event.setEventType(m_type == 'm' ? EventType::MouseRelease : EventType::MousePress);
                event.setMouseButton(static_cast<MouseButton>(button & 0b11));
                event.m_x = x - 1;
                event.m_y = y - 1;

                event.m_globalX = x - 1;
                event.m_globalY = y - 1;

                event.setModifiers(
                    button & (1 << 2),  // Shift
                    button & (1 << 3),  // Alt
                    button & (1 << 4)   // Ctrl
                );
            } else {
                event.setEventType(EventType::Unknown);
            }
            return event;
        }

        event.setEventType(EventType::Escape);
        return event;
    }

    if (data[0] == 3) { // Ctrl+C
        event.setEventType(EventType::Quit);
        return event;
    }

    event.setEventType(EventType::KeyPress);
    event.setKey(std::string(1, data[0]));
    return event;
}

}
