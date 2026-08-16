#pragma once
#include <string>
#include "jobsound_export.h"
namespace job::sound {
class JOBSOUND_EXPORT AudioEffect
{
public:
    AudioEffect(std::string uid) :
        m_uid{uid}
    {

    }
    virtual ~AudioEffect() {}
    [[nodiscard]] virtual float process(float input) = 0;
    std::string uid() const
    {
        return m_uid;
    }
private:
    std::string m_uid = "unknown";
};


}
