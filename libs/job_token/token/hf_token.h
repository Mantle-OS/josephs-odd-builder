#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "itoken.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT HfToken final : public IToken
{
public:
    using Ptr  = std::shared_ptr<HfToken>;
    using WPtr = std::weak_ptr<HfToken>;
    using UPtr = std::unique_ptr<HfToken>;

    using Merges = std::vector<std::pair<std::string, std::string>>;

    HfToken()
    {
        setProvider(Provider::HuggingFace);
    }

    ~HfToken() override = default;

    HfToken(const HfToken &) = delete;
    HfToken &operator=(const HfToken &) = delete;
    HfToken(HfToken &&) = delete;
    HfToken &operator=(HfToken &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<HfToken>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<HfToken>();
    }

    [[nodiscard]] bool load(const std::filesystem::path &json, const std::filesystem::path &config = {});
    [[nodiscard]] bool load(std::string_view json, std::string_view config = {});
    [[nodiscard]] bool loadJson(std::string_view json);
    [[nodiscard]] bool loadConfig(const std::filesystem::path &config);
    [[nodiscard]] bool loadConfigJson(std::string_view json);

    [[nodiscard]] const Merges &merges() const noexcept
    {
        return m_merges;
    }

    [[nodiscard]] bool cleanUpTokenizationSpaces() const noexcept
    {
        return m_cleanUpTokenizationSpaces;
    }

    void setCleanUpTokenizationSpaces(bool enabled) noexcept
    {
        m_cleanUpTokenizationSpaces = enabled;
    }

protected:
    std::string tokenString(const nlohmann::json &value);
    void extraClear() noexcept override;

private:
    struct HfAddedToken
    {
        TokenId id{kInvalidToken};
        std::string content;

        bool special{false};
        bool singleWord{false};
        bool lstrip{false};
        bool rstrip{false};
        bool normalized{false};
    };

private:
    Merges m_merges;
    std::vector<HfAddedToken> m_addedTokens;

    bool m_cleanUpTokenizationSpaces{true};
};

} // namespace job::token