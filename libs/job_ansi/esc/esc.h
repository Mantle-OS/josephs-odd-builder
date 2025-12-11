#pragma once

#include <unordered_map>
#include <functional>

#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"

namespace job::ansi::csi {
class DispatchESC {
public:
    explicit DispatchESC(Screen *screen);
    void dispatch(ESC_CODE code, CharsetDesignator designator);

private:
    Screen                                                                  *m_screen = nullptr;
    std::unordered_map<ESC_CODE, std::function<void(CharsetDesignator)>>    m_handlers;
    void initHandlers();
};
}
