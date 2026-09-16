#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <streambuf>
#include <type_traits>
#include <utility>
#include <vector>

#include "job_utf_stream_concept.h"
#include "job_utf8.h"
#include "job_utf16.h"
#include "job_utf32.h"

namespace job::core::text {

template <UtfStreamCodec Codec>
class UtfStreamBuf : public std::basic_streambuf<
                         std::conditional_t<std::same_as<typename Codec::UnitInput::value_type,std::uint8_t>,char, typename Codec::UnitInput::value_type>>
{


public:
    using Ptr       = std::shared_ptr<UtfStreamBuf>;
    using WPtr      = std::weak_ptr<UtfStreamBuf>;
    using UPtr      = std::unique_ptr<UtfStreamBuf>;

    using CodecType = Codec;
    using Unit      = typename Codec::UnitInput::value_type;

    using CharType  = std::conditional_t<std::same_as<Unit, std::uint8_t>,char,Unit>;

    using Base       = std::basic_streambuf<CharType>;
    using TraitsType = typename Base::traits_type;
    using IntType    = typename Base::int_type;
    using PosType    = typename Base::pos_type;
    using OffType    = typename Base::off_type;

    using ReadView  = std::span<const CharType>;
    using WriteView = std::span<CharType>;

    using WriteCallback = std::function<std::streamsize(const CharType *data, std::streamsize count)>;

    UtfStreamBuf();
    ~UtfStreamBuf() override;

    UtfStreamBuf(const UtfStreamBuf &) = delete;
    UtfStreamBuf &operator=(const UtfStreamBuf &) = delete;
    UtfStreamBuf(UtfStreamBuf &&) = delete;
    UtfStreamBuf &operator=(UtfStreamBuf &&) = delete;

    template <typename... Args>
        requires std::constructible_from<UtfStreamBuf, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<UtfStreamBuf>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<UtfStreamBuf, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<UtfStreamBuf>(std::forward<Args>(args)...);
    }

    [[nodiscard]] std::streamsize feed(const CharType *data, std::streamsize count);
    [[nodiscard]] std::streamsize feed(ReadView input);

    void setWriteCallback(WriteCallback callback);

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    void clear();

protected:
    [[nodiscard]] IntType underflow() override;
    [[nodiscard]] IntType overflow(IntType ch = TraitsType::eof()) override;

    int sync() override;

    [[nodiscard]] std::streamsize xsputn(const CharType *data, std::streamsize count) override;
    [[nodiscard]] std::streamsize xsgetn(CharType *data, std::streamsize count) override;

    [[nodiscard]] std::streamsize showmanyc() override;

private:
    [[nodiscard]] static typename Codec::UnitInput unitInput(ReadView input) noexcept;
    [[nodiscard]] static typename Codec::UnitOutput unitOutput(WriteView output) noexcept;
    [[nodiscard]] std::streamsize appendInput(ReadView input);

    void resetGetArea() noexcept;
    std::vector<CharType> m_inputBuffer;
    std::size_t m_inputOffset{};
    WriteCallback m_writeCallback;
};

using Utf8StreamBuf  = UtfStreamBuf<Utf8>;
using Utf16StreamBuf = UtfStreamBuf<Utf16>;
using Utf32StreamBuf = UtfStreamBuf<Utf32>;

extern template class UtfStreamBuf<Utf8>;
extern template class UtfStreamBuf<Utf16>;
extern template class UtfStreamBuf<Utf32>;

} // namespace job::core::text