#pragma once

#include <cassert>
#include <cstdint>

#include "view.h"

namespace job::ai::cords {

class Batch : public ViewR {
public:
    using View   = ViewR;
    using Extent = View::Extent;
    using View::View;

    Batch(core::real_t *data, std::uint32_t batch, std::uint32_t channel, std::uint32_t height, std::uint32_t width) :
        View(data, Extent{batch, channel, height, width})
    {
        assert(rank() == 4);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t batch, std::uint32_t channel, std::uint32_t height, std::uint32_t width)
    {
        assert(rank() == 4);
        assert(batch < m_extent[0] && channel < m_extent[1]
               && height < m_extent[2] && width < m_extent[3]);
        return (*this)(batch, channel, height, width);
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t batch, std::uint32_t channel,
                                         std::uint32_t height, std::uint32_t width) const
    {
        assert(rank() == 4);
        assert(batch < m_extent[0] && channel < m_extent[1]
               && height < m_extent[2] && width < m_extent[3]);
        return (*this)(batch, channel, height, width);
    }

    [[nodiscard]] std::uint32_t batch() const
    {
        return m_extent[0];
    }
    [[nodiscard]] std::uint32_t channels() const
    {
        return m_extent[1];
    }
    [[nodiscard]] std::uint32_t height() const
    {
        return m_extent[2];
    }
    [[nodiscard]] std::uint32_t width() const
    {
        return m_extent[3];
    }
};

} // namespace job::ai::cords
