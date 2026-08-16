#include "feed_forward_config.h"

namespace job::model {

FeedForwardConfig::FeedForwardConfig() = default;

bool FeedForwardConfig::isValid() const noexcept
{
    // A zero-width FFN can't do anything useful.
    return m_feedForwardLength > 0;
}

} // namespace job::model