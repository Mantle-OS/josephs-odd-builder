#pragma once

#include <string>
#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"

// update like the rest ?

namespace job::ansi::csi {

class DispatchOSC {
public:
    explicit DispatchOSC(Screen *screen);
    void dispatch(OSC_CODE code, const std::string &data);

private:
    Screen *m_screen;
};

}
