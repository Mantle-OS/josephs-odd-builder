#pragma once

#include <vector>

#include "job_ansi_screen.h"

#include "job_ansi_attributes.h"


namespace job::ansi::csi {

class DispatchSGR {
public:
    explicit DispatchSGR(Screen *screen);
    void dispatch(Attributes *attributes, const std::vector<int> &params);

private:
    Screen *m_screen;
};

}
