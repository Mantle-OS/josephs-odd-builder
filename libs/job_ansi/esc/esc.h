#pragma once

#include "csi/dispatch_base.h"
#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"

// update like the rest...

namespace job::ansi::csi {
class DispatchESC final : public DispatchBase<ESC_CODE, CharsetDesignator> {
public:
    explicit DispatchESC(Screen *screen);
    void dispatch(ESC_CODE code, CharsetDesignator designator) override;
    void initMap() override;
};
}
