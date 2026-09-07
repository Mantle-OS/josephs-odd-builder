#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace job::yaml {

class YamlDecodedScalar
{
public:
    using Ptr = std::shared_ptr<YamlDecodedScalar>;
    using WPtr = std::weak_ptr<YamlDecodedScalar>;
    using UPtr = std::unique_ptr<YamlDecodedScalar>;

    YamlDecodedScalar() = default;
    ~YamlDecodedScalar() = default;

    YamlDecodedScalar(const YamlDecodedScalar &) = default;
    YamlDecodedScalar &operator=(const YamlDecodedScalar &) = default;

    YamlDecodedScalar(YamlDecodedScalar &&) noexcept = default;
    YamlDecodedScalar &operator=(YamlDecodedScalar &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<YamlDecodedScalar>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<YamlDecodedScalar>();
    }

    [[nodiscard]] std::string_view value() const noexcept
    {
        return m_transformed ? std::string_view{m_storage} : m_view;
    }

    [[nodiscard]] bool transformed() const noexcept
    {
        return m_transformed;
    }

    [[nodiscard]] bool borrowed() const noexcept
    {
        return !m_transformed;
    }

    void setBorrowed(std::string_view value) noexcept
    {
        m_view = value;
        m_storage.clear();
        m_transformed = false;
    }

    void setTransformed(std::string value)
    {
        m_view = {};
        m_storage = std::move(value);
        m_transformed = true;
    }

    void clear() noexcept
    {
        m_view = {};
        m_storage.clear();
        m_transformed = false;
    }

private:
    std::string_view m_view{};
    std::string m_storage{};
    bool m_transformed{};
};

} // namespace job::yaml