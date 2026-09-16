#pragma once

#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"

namespace job::ansi::csi {
class DispatchDECSET {
public:
    explicit DispatchDECSET(Screen *screen) :
        m_screen(screen)
    {
    }

    void dispatchPrivate(DECSET_PRIVATE_CODE code, bool set);
    void dispatchPublic(DECSET_PUBLIC_CODE code, bool set);

private:
    Screen *m_screen = nullptr;

    // FIXME: future maps ? might not be worth it
    // std::unordered_map<DECSET_PRIVATE_CODE, std::function<void(bool)>> m_privateHandlers;
    // std::unordered_map<DECSET_PUBLIC_CODE, std::function<void(bool)>> m_publicHandlers;
};
}

