#include "device_config.h"

namespace job::model {

DeviceConfig::DeviceConfig() = default;

bool DeviceConfig::isValid() const noexcept
{
    // Every field here is either an "auto"-sentinel-friendly number (0 =
    // no budget/no thread limit) or a bool -- nothing to reject yet.
    // useMLock without useMmap is unusual but not nonsensical (mlocking
    // a heap-allocated buffer is a real thing too), so not rejected here.
    return true;
}

} // namespace job::model