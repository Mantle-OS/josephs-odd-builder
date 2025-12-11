#include "job_tui_core_application.h"

#include <algorithm>
#include <csignal>

#include <fcntl.h>

#include <iostream>

#include <job_ansi_suffix.h>

#include "job_tui_draw_context.h"

namespace {
static void collectFocusableItems(const job::tui::JobTuiItem::Ptr &node, std::vector<job::tui::JobTuiItem::Ptr> &out)
{
    if (node->isVisible() && node->isFocusable())
        out.push_back(node);

    for (auto &[_, child] : node->children()) {
        // JobTuiObject::Ptr
        auto shared = child;
        auto item = std::dynamic_pointer_cast<job::tui::JobTuiItem>(shared);
        if (item)
            collectFocusableItems(item, out);
    }
}
} // namespace

namespace job::tui {

JobTuiCoreApplication::JobTuiCoreApplication(core::IODevice::Ptr device) :
    m_device(std::move(device))
{
}

JobTuiCoreApplication::~JobTuiCoreApplication()
{
    stop();
    restoreTerminal();
}

void JobTuiCoreApplication::setRoot(JobTuiItem::Ptr root)
{
    m_root = std::move(root);
    if (m_root)
        m_root->setGeometry(0, 0, m_width, m_height);
}

void JobTuiCoreApplication::setWindow( JobTuiItem::Ptr window)
{
    setRoot(window);
}

void JobTuiCoreApplication::addChild(JobTuiItem::Ptr widget)
{
    m_children.push_back(std::move(widget));
}

void JobTuiCoreApplication::removeChild(JobTuiItem *widget)
{
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
                       [widget](const std::shared_ptr<JobTuiItem> &ptr) {
                           return ptr.get() == widget;
                       }),
        m_children.end());
}

void JobTuiCoreApplication::resize(int width, int height)
{
    m_width = width;
    m_height = height;
    if (m_root)
        m_root->setGeometry(0, 0, width, height);
}


bool JobTuiCoreApplication::render()
{
    if (!m_root || !m_device || !m_device->isOpen())
        return false;

    std::string output;
    output += ansi::utils::suffix::CURSOR_HIDE;
    output += ansi::utils::suffix::CURSOR_SAVE;
    output += ansi::utils::suffix::CURSOR_HOME;

    DrawContext ctx(m_device);
    m_root->paint(ctx);
    for (const auto &child : m_children)
        if (child && child->isVisible())
            child->paint(ctx);

    output += ansi::utils::suffix::RESET_SGR;
    output += ansi::utils::suffix::CURSOR_RESTORE;

    if (m_showCursor)
        output += ansi::utils::suffix::CURSOR_SHOW;
    else
        output += ansi::utils::suffix::CURSOR_HIDE;

    return m_device->write(output.data(), output.size()) >= 0;
}


void JobTuiCoreApplication::markDirty()
{
    m_dirty.store(true);
}

void JobTuiCoreApplication::handleEvent(const Event &event)
{
    if (!m_root)
        return;

    m_root->handleEvent(event);
}


bool JobTuiCoreApplication::pollNativeEvent(Event &out)
{
    char buf[16];
    ssize_t bytes = ::read(STDIN_FILENO, buf, sizeof(buf));
    if (bytes > 0) {
        out = Event::fromRawInput(buf, bytes);
        return true;
    }

    return false;
}

void JobTuiCoreApplication::handleFocus(const Event &event)
{
    // Always let the focused item try first
    if (m_focusedItem && m_focusedItem->focusEvent(event)) {
        // But if it's TAB, we still want to advance focus
        if (event.type() == Event::EventType::KeyPress && event.key() == "\t") {
            focusNext();
            markDirty();
        }
        return;
    }

    // If no focused item handled it, fallback handlers
    if (event.type() == Event::EventType::KeyPress && event.key() == "\t") {
        focusNext();
        markDirty();
        return;
    }

    if (m_root && m_root->focusEvent(event))
        return;

    for (const auto &child : m_children)
        if (child && child->isVisible() && child->focusEvent(event))
            return;

}

void JobTuiCoreApplication::setupRawMode()
{
    if (!isatty(STDIN_FILENO))
        return;

    if (tcgetattr(STDIN_FILENO, &m_origTermios) == -1)
        return;

    termios raw = m_origTermios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
        m_rawModeEnabled = true;

    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void JobTuiCoreApplication::restoreTerminal()
{
    if (m_rawModeEnabled)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_origTermios);

    m_rawModeEnabled = false;
}

void JobTuiCoreApplication::start()
{
    m_showCursor = false;
    m_running.store(true);
    setupRawMode();

    if(m_device->write(ansi::utils::suffix::ENABLE_MOUSE_ALL.c_str()) < 0)
        std::cout << "UNABLE to write enable mouse";

    if (!m_focusedItem)
        focusNext();
}

void JobTuiCoreApplication::stop()
{
    m_running.store(false);

    if(m_device->write(ansi::utils::suffix::DISABLE_MOUSE_ALL.c_str()) < 0)
        std::cout << "UNABLE to write disable mouse";

    restoreTerminal();
}

bool JobTuiCoreApplication::isRunning() const
{
    return m_running.load();
}

int JobTuiCoreApplication::exec()
{
    start();

    m_running.store(true);
    m_lastRenderTime = std::chrono::steady_clock::now();

    m_device->write(ansi::utils::suffix::ENABLE_ALT_SCREEN);

    while (m_running.load()) {
        Event ev;
        if (pollNativeEvent(ev)) {
            if (ev.type() == Event::EventType::Quit)
                break;
            handleFocus(ev);
            handleEvent(ev);
            markDirty(); // redraw after input
        }

        auto now = std::chrono::steady_clock::now();
        bool forceRender = (now - m_lastRenderTime) >= m_minRenderInterval;

        if (m_dirty.load() || forceRender) {
            render();
            m_lastRenderTime = now;
            m_dirty.store(false);
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // 30ms = ~33 FPS max
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // 30ms = ~33 FPS max
    }

    m_device->write(ansi::utils::suffix::DISABLE_ALT_SCREEN);
    return m_exitCode;
}

void JobTuiCoreApplication::focusNext()
{
    std::vector<JobTuiItem::Ptr> focusables;
    collectFocusableItems(m_root, focusables);

    if (focusables.empty())
        return;

    auto it = std::find(focusables.begin(), focusables.end(), m_focusedItem);
    if (it != focusables.end()) {
        (*it)->setFocus(false);
        ++it;
        if (it == focusables.end())
            it = focusables.begin();
    } else {
        it = focusables.begin();
    }

    m_focusedItem = *it;
    m_focusedItem->setFocus(true);
    markDirty();
}


void JobTuiCoreApplication::focusPrevious()
{
    std::vector<JobTuiItem::Ptr> focusables;

    if (m_root && m_root->isVisible() && m_root->isFocusable())
        focusables.push_back(m_root);

    for (const auto &child : m_children)
        if (child && child->isVisible() && child->isFocusable())
            focusables.push_back(child);

    if (focusables.empty())
        return;

    auto it = std::find(focusables.begin(), focusables.end(), m_focusedItem);
    if (it != focusables.end()) {
        (*it)->setFocus(false);
        if (it == focusables.begin())
            it = focusables.end();
        --it;
    } else {
        it = std::prev(focusables.end());
    }

    m_focusedItem = *it;
    m_focusedItem->setFocus(true);
    markDirty();
}

bool JobTuiCoreApplication::showCursor() const
{
    return m_showCursor;
}

void JobTuiCoreApplication::setShowCursor(bool showCursor)
{
    m_showCursor = showCursor;
}

void JobTuiCoreApplication::exit(int code)
{
    m_exitCode = code;
    stop();
}

void JobTuiCoreApplication::quit()
{
    m_exitCode = 0;
    stop();
}


}
