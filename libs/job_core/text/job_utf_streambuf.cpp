#include "job_utf_streambuf.h"

#include <algorithm>
#include <limits>

namespace job::core::text {

template <UtfStreamCodec Codec>
UtfStreamBuf<Codec>::UtfStreamBuf()
{
    resetGetArea();
}

template <UtfStreamCodec Codec>
UtfStreamBuf<Codec>::~UtfStreamBuf() = default;

template <UtfStreamCodec Codec>
std::streamsize UtfStreamBuf<Codec>::feed(const CharType *data, std::streamsize count)
{
    if (!data || count <= 0)
        return 0;

    return appendInput(ReadView{data, static_cast<std::size_t>(count)});
}

template <UtfStreamCodec Codec>
std::streamsize UtfStreamBuf<Codec>::feed(ReadView input)
{
    if (input.empty())
        return 0;

    return appendInput(input);
}

template <UtfStreamCodec Codec>
void UtfStreamBuf<Codec>::setWriteCallback(WriteCallback callback)
{
    m_writeCallback = std::move(callback);
}

template <UtfStreamCodec Codec>
bool UtfStreamBuf<Codec>::empty() const noexcept
{
    if (!this->gptr() || !this->egptr())
        return true;

    return this->gptr() == this->egptr();
}

template <UtfStreamCodec Codec>
std::size_t UtfStreamBuf<Codec>::size() const noexcept
{
    if (!this->gptr() || !this->egptr())
        return 0;

    return static_cast<std::size_t>(this->egptr() - this->gptr());
}

template <UtfStreamCodec Codec>
void UtfStreamBuf<Codec>::clear()
{
    m_inputBuffer.clear();
    m_inputOffset = 0;
    resetGetArea();
}

template <UtfStreamCodec Codec>
typename UtfStreamBuf<Codec>::IntType UtfStreamBuf<Codec>::underflow()
{
    if (!this->gptr() || this->gptr() == this->egptr())
        return TraitsType::eof();

    return TraitsType::to_int_type(*this->gptr());
}

template <UtfStreamCodec Codec>
typename UtfStreamBuf<Codec>::IntType UtfStreamBuf<Codec>::overflow(IntType ch)
{
    if (TraitsType::eq_int_type(ch, TraitsType::eof()))
        return TraitsType::not_eof(ch);

    const CharType value = TraitsType::to_char_type(ch);
    if (xsputn(&value, 1) != 1)
        return TraitsType::eof();

    return ch;
}

template <UtfStreamCodec Codec>
int UtfStreamBuf<Codec>::sync()
{
    return 0;
}

template <UtfStreamCodec Codec>
std::streamsize UtfStreamBuf<Codec>::xsputn(const CharType *data, std::streamsize count)
{
    if (!data || count <= 0 || !m_writeCallback)
        return 0;

    const std::streamsize written = m_writeCallback(data, count);
    if (written <= 0)
        return 0;

    return std::min(written, count);
}

template <UtfStreamCodec Codec>
std::streamsize UtfStreamBuf<Codec>::xsgetn(CharType *data, std::streamsize count)
{
    if (!data || count <= 0 || !this->gptr() || !this->egptr())
        return 0;

    const std::streamsize available = static_cast<std::streamsize>(this->egptr() - this->gptr());
    const std::streamsize readCount = std::min(count, available);
    if (readCount <= 0)
        return 0;

    TraitsType::copy(data, this->gptr(), static_cast<std::size_t>(readCount));
    std::streamsize remaining = readCount;
    while (remaining > 0) {
        constexpr std::streamsize maxBump =
            static_cast<std::streamsize>(std::numeric_limits<int>::max());

        const int bump = static_cast<int>(std::min(remaining, maxBump));
        this->gbump(bump);
        remaining -= bump;
    }

    return readCount;
}

template <UtfStreamCodec Codec>
std::streamsize UtfStreamBuf<Codec>::showmanyc()
{
    if (!this->gptr() || !this->egptr())
        return 0;

    return static_cast<std::streamsize>(this->egptr() - this->gptr());
}

template <UtfStreamCodec Codec>
typename Codec::UnitInput UtfStreamBuf<Codec>::unitInput(ReadView input) noexcept
{
    if constexpr (std::same_as<Unit, std::uint8_t>) {
        static_assert(sizeof(CharType) == sizeof(Unit));

        return typename Codec::UnitInput{
            reinterpret_cast<const Unit *>(input.data()),
            input.size()
        };
    } else {
        return typename Codec::UnitInput{input.data(), input.size()};
    }
}

template <UtfStreamCodec Codec>
typename Codec::UnitOutput UtfStreamBuf<Codec>::unitOutput(WriteView output) noexcept
{
    if constexpr (std::same_as<Unit, std::uint8_t>) {
        static_assert(sizeof(CharType) == sizeof(Unit));

        return typename Codec::UnitOutput{
            reinterpret_cast<Unit *>(output.data()),
            output.size()
        };
    } else {
        return typename Codec::UnitOutput{output.data(), output.size()};
    }
}

template <UtfStreamCodec Codec>
std::streamsize UtfStreamBuf<Codec>::appendInput(ReadView input)
{
    if (input.empty())
        return 0;

    if (this->eback() && this->gptr()) {
        m_inputOffset =
            static_cast<std::size_t>(this->gptr() - this->eback());
    }

    if (m_inputOffset > 0) {
        m_inputBuffer.erase(
            m_inputBuffer.begin(),
            m_inputBuffer.begin() +
                static_cast<std::ptrdiff_t>(m_inputOffset));

        m_inputOffset = 0;
    }

    m_inputBuffer.insert(
        m_inputBuffer.end(),
        input.begin(),
        input.end());

    resetGetArea();

    return static_cast<std::streamsize>(input.size());
}

template <UtfStreamCodec Codec>
void UtfStreamBuf<Codec>::resetGetArea() noexcept
{
    if (m_inputBuffer.empty()) {
        this->setg(nullptr, nullptr, nullptr);
        return;
    }

    CharType *begin = m_inputBuffer.data();
    CharType *current = begin + m_inputOffset;
    CharType *end = begin + m_inputBuffer.size();

    this->setg(begin, current, end);
}

template class UtfStreamBuf<Utf8>;
template class UtfStreamBuf<Utf16>;
template class UtfStreamBuf<Utf32>;

} // namespace job::core::text