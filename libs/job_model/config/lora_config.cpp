#include "lora_config.h"

#include <cstddef>
#include <stdexcept>

#include <real_type.h>

namespace job::model {

Lora::Lora() = default;

void Lora::setMultiplier(float value)
{
    if (!core::isSafeFinite(value)) {
        throw std::invalid_argument{"Lora multiplier must be finite"};
    }

    m_multiplier = value;
}

bool Lora::isValid() const noexcept
{
    return core::isSafeFinite(m_multiplier) && !m_path.empty();
}

LoraConfig::LoraConfig() = default;

void LoraConfig::append(Lora lora)
{
    m_loras.append(std::move(lora));
}

void LoraConfig::prepend(Lora lora)
{
    m_loras.prepend(std::move(lora));
}

void LoraConfig::disable(int x)
{
    // JobList::at() throws std::out_of_range for any index past the end
    // -- including a negative x, which wraps to a huge value once cast
    // to the unsigned size_type, so no separate x < 0 check is needed.
    m_loras.at(static_cast<core::JobList<Lora>::size_type>(x)).setEnabled(false);
}

bool LoraConfig::isValid() const noexcept
{
    for (const Lora &lora : m_loras) {
        if (lora.isEnabled() && !lora.isValid())
            return false;
    }

    return true;
}

} // namespace job::model