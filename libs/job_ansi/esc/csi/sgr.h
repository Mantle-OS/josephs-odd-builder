#pragma once
#include "job_ansi_screen.h"
namespace job::ansi::csi {
class DispatchSGR {
public:
    explicit DispatchSGR(Screen *screen);
    void dispatch(std::span<const int> params);
private:
    Screen *m_screen;
};
}
