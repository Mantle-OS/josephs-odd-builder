#pragma once

#include <unordered_map>
#include <functional>
#include <span>

#include <job_logger.h>
// Forward declaration is sufficient here
namespace job::ansi { class Screen; }

namespace job::ansi::csi {

template<typename CodeEnum, typename ParamT = std::span<const int>>
class DispatchBase {
public:
    using Handler = std::function<void(ParamT)>;

    explicit DispatchBase(ansi::Screen *screen = nullptr) :
        m_screen(screen)
    {
    }

    virtual ~DispatchBase() = default;
    DispatchBase(const DispatchBase &) = delete;
    DispatchBase &operator=(const DispatchBase &) = delete;

    virtual void dispatch(CodeEnum code, ParamT params)
    {
        if (auto it = m_dispatchMap.find(code); it != m_dispatchMap.end()) {
            it->second(params);
        } else {
            onUnhandled(code);
        }
    }

    virtual void initMap() = 0;
    void setScreen(ansi::Screen *screen) { m_screen = screen; }

protected:
    // Hook for unhandled codes (cleaner than raw cerr in the logic)
    virtual void onUnhandled(CodeEnum code)
    {
        JOB_LOG_DEBUG("[Dispatch] Unhandled code: {}", static_cast<int>(code));
    }

    job::ansi::Screen *m_screen = nullptr;
    std::unordered_map<CodeEnum, Handler> m_dispatchMap;
};

} // namespace job::ansi::csi
