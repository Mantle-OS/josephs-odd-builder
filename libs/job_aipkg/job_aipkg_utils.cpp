#include "job_aipkg_utils.h"

namespace job::aipkg {

size_t largestPowerOfTwoLessThan(size_t n) noexcept
{
    size_t k = 1;
    while (k * 2 < n)
        k *= 2;
    return k;
}


} // namespace job::aipkg