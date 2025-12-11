#pragma once
#include "job_tui_object.h"
namespace job::tui {
class JobTuiAnimation : public JobTuiObject {
public:
    virtual ~JobTuiAnimation() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual bool isDone() const = 0;
    virtual void update(int elapsedMs) = 0;
};
}
