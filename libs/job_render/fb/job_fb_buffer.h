#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "job_fb_visual.h"
#include "job_render_format_type.h"

namespace job::render {

class JobFbBuffer
{
public:
    using Ptr  = std::shared_ptr<JobFbBuffer>;
    using WPtr = std::weak_ptr<JobFbBuffer>;
    using UPtr = std::unique_ptr<JobFbBuffer>;

    using Size = std::size_t;

    JobFbBuffer() = default;

    JobFbBuffer(std::byte *data,
                Size size, std::uint32_t width, std::uint32_t height, std::uint32_t virtualWidth, std::uint32_t virtualHeight,
                std::uint32_t xOffset, std::uint32_t yOffset,
                std::uint32_t stride,
                std::uint32_t bitsPerPixel,
                FormatType format,
                JobFbVisual visual) noexcept :
        m_data(data),
        m_size(size),
        m_width(width),
        m_height(height),
        m_virtualWidth(virtualWidth),
        m_virtualHeight(virtualHeight),
        m_xOffset(xOffset),
        m_yOffset(yOffset),
        m_stride(stride),
        m_bitsPerPixel(bitsPerPixel),
        m_format(format),
        m_visual(visual)
    {
    }

    ~JobFbBuffer() = default;

    JobFbBuffer(const JobFbBuffer &) = default;
    JobFbBuffer &operator=(const JobFbBuffer &) = default;

    JobFbBuffer(JobFbBuffer &&) noexcept = default;
    JobFbBuffer &operator=(JobFbBuffer &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(std::byte *data,
                                          Size size,
                                          std::uint32_t width,
                                          std::uint32_t height,
                                          std::uint32_t virtualWidth,
                                          std::uint32_t virtualHeight,
                                          std::uint32_t xOffset,
                                          std::uint32_t yOffset,
                                          std::uint32_t stride,
                                          std::uint32_t bitsPerPixel,
                                          FormatType format,
                                          JobFbVisual visual)
    {
        return std::make_shared<JobFbBuffer>(data,
                                             size,
                                             width,
                                             height,
                                             virtualWidth,
                                             virtualHeight,
                                             xOffset,
                                             yOffset,
                                             stride,
                                             bitsPerPixel,
                                             format,
                                             visual);
    }

    [[nodiscard]] static UPtr createUniq(std::byte *data,
                                         Size size,
                                         std::uint32_t width,
                                         std::uint32_t height,
                                         std::uint32_t virtualWidth,
                                         std::uint32_t virtualHeight,
                                         std::uint32_t xOffset,
                                         std::uint32_t yOffset,
                                         std::uint32_t stride,
                                         std::uint32_t bitsPerPixel,
                                         FormatType format,
                                         JobFbVisual visual)
    {
        return std::make_unique<JobFbBuffer>(data,
                                             size,
                                             width,
                                             height,
                                             virtualWidth,
                                             virtualHeight,
                                             xOffset,
                                             yOffset,
                                             stride,
                                             bitsPerPixel,
                                             format,
                                             visual);
    }

    [[nodiscard]] std::byte *data() noexcept
    {
        return m_data;
    }

    [[nodiscard]] const std::byte *data() const noexcept
    {
        return m_data;
    }

    [[nodiscard]] Size size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] std::span<std::byte> bytes() noexcept
    {
        return {m_data, m_size};
    }

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept
    {
        return {m_data, m_size};
    }

    [[nodiscard]] std::uint32_t width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] std::uint32_t height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] std::uint32_t virtualWidth() const noexcept
    {
        return m_virtualWidth;
    }

    [[nodiscard]] std::uint32_t virtualHeight() const noexcept
    {
        return m_virtualHeight;
    }

    [[nodiscard]] std::uint32_t xOffset() const noexcept
    {
        return m_xOffset;
    }

    [[nodiscard]] std::uint32_t yOffset() const noexcept
    {
        return m_yOffset;
    }

    [[nodiscard]] std::uint32_t stride() const noexcept
    {
        return m_stride;
    }

    [[nodiscard]] std::uint32_t bitsPerPixel() const noexcept
    {
        return m_bitsPerPixel;
    }

    [[nodiscard]] FormatType format() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] JobFbVisual visual() const noexcept
    {
        return m_visual;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_data           != nullptr &&
               m_size           != 0 &&
               m_width          != 0 &&
               m_height         != 0 &&
               m_virtualWidth   != 0 &&
               m_virtualHeight  != 0 &&
               m_xOffset        <= m_virtualWidth &&
               m_yOffset        <= m_virtualHeight &&
               m_width          <= m_virtualWidth - m_xOffset &&
               m_height         <= m_virtualHeight - m_yOffset &&
               m_stride         != 0 &&
               m_bitsPerPixel   != 0 &&
               m_format         != FormatType::Unknown;
    }

private:
    std::byte       *m_data{}; // BORROWED
    Size            m_size{};
    std::uint32_t   m_width{};
    std::uint32_t   m_height{};
    std::uint32_t   m_virtualWidth{};
    std::uint32_t   m_virtualHeight{};
    std::uint32_t   m_xOffset{};
    std::uint32_t   m_yOffset{};
    std::uint32_t   m_stride{};
    std::uint32_t   m_bitsPerPixel{};
    FormatType      m_format{FormatType::Unknown};
    JobFbVisual     m_visual;
};

}