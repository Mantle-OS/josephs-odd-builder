#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

#include "job_ansi_screen.h"

namespace job::ansi::csi {
using namespace job::ansi::utils::enums;
template<typename CodeEnum>
class DispatchBase {
public:
    using Handler = std::function<void(const std::vector<int> &)>;

    explicit DispatchBase(Screen *screen = nullptr) :
        m_screen(screen)
    {

    }
    virtual ~DispatchBase() = default;

    // Prevent accidental copying
    DispatchBase(const DispatchBase &) = delete;
    DispatchBase &operator=(const DispatchBase &) = delete;

    virtual void dispatch(CodeEnum code, const std::vector<int> &params)
    {
        if (m_dispatchMap.contains(code))
            m_dispatchMap.at(code)(params);
        else
            std::cerr << "Unhandled CSI code: " << static_cast<int>(code) << '\n';
    }

    virtual void initMap() = 0;

    // Optional: for testing/debugging
    const std::unordered_map<CodeEnum, Handler> &handlers() const
    {
        return m_dispatchMap;
    }

protected:
    Screen                                      *m_screen = nullptr;
    std::unordered_map<CodeEnum, Handler>       m_dispatchMap;
};

}
