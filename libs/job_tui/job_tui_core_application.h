#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <termios.h>

#include <io_base.h>

#include "job_tui_event.h"
#include "job_tui_item.h"

namespace job::tui {
class JobTuiCoreApplication {
public:
    explicit JobTuiCoreApplication(core::IODevice::Ptr device);
    ~JobTuiCoreApplication();

    void setRoot(JobTuiItem::Ptr root);
    void setWindow(JobTuiItem::Ptr window);
    void addChild(JobTuiItem::Ptr widget);
    void removeChild(JobTuiItem *widget);

    void resize(int width, int height);
    bool render();
    void markDirty();

    void handleEvent(const Event &event);
    void handleFocus(const Event &event);
    bool pollNativeEvent(Event &out);

    void start();
    void stop();
    void quit();
    void exit(int code);
    bool isRunning() const;

    int exec();

    // Thread coordination — move all children and root to a specific thread
    void moveAllToThread(std::thread::id id);

    void focusNext();
    void focusPrevious();

    bool showCursor() const;
    void setShowCursor(bool showCursor);

private:
    job::core::IODevice::Ptr        m_device;
    JobTuiItem::Ptr                 m_root;
    std::vector<JobTuiItem::Ptr>    m_children;
    JobTuiItem::Ptr                 m_focusedItem;
    int                             m_width = 80;
    int                             m_height = 25;
    int                             m_exitCode = 0;
    bool                            m_showCursor;
    std::atomic<bool>               m_running{false};
    struct termios                  m_origTermios;
    bool                            m_rawModeEnabled = false;

    void setupRawMode();
    void restoreTerminal();
    std::atomic<bool> m_dirty = true;
    std::chrono::steady_clock::time_point m_lastRenderTime;
    std::chrono::milliseconds m_minRenderInterval{1500};
};

}
