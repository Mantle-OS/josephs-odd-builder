#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_gguf.h>

#include "itoken.h"
#include "job_token_enums.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT GgufToken final : public IToken
{
public:
    using Ptr  = std::shared_ptr<GgufToken>;
    using WPtr = std::weak_ptr<GgufToken>;
    using UPtr = std::unique_ptr<GgufToken>;

    using Merges = std::vector<std::pair<std::string, std::string>>;

    GgufToken() :
        m_gguf{ggml::JobGguf::createUniq()}
    {
        setProvider(Provider::Gguf);
    }

    ~GgufToken() override = default;

    GgufToken(const GgufToken &) = delete;
    GgufToken &operator=(const GgufToken &) = delete;
    GgufToken(GgufToken &&) = delete;
    GgufToken &operator=(GgufToken &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<GgufToken>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<GgufToken>();
    }

    [[nodiscard]] bool load(const std::filesystem::path &path);
    [[nodiscard]] bool load(const void *data, std::size_t size);
    [[nodiscard]] bool load(std::span<const std::byte> buffer);

    [[nodiscard]] const std::string &modelName() const noexcept
    {
        return m_modelName;
    }

    [[nodiscard]] const std::string &preTokenizer() const noexcept
    {
        return m_preTokenizer;
    }

    [[nodiscard]] const Merges &merges() const noexcept
    {
        return m_merges;
    }

protected:
    void extraClear() noexcept override
    {
        setProvider(Provider::Gguf);

        m_modelName.clear();
        m_preTokenizer.clear();
        m_merges.clear();
    }

private:
    [[nodiscard]] bool load();

    template<std::size_t N>
    [[nodiscard]] static constexpr bool containsName(const std::array<std::string_view, N> &names, std::string_view name) noexcept
    {
        for (const std::string_view candidate : names)
            if (candidate == name)
                return true;

        return false;
    }

    struct PreTokenizerMap
    {
        std::string_view name;
        SplitPattern pattern;
    };
    // Cannon: International Museum of Tokenizer String Aliases™
    inline static constexpr std::array<PreTokenizerMap, 13> kPreTokenizerMap = {{
                                                                                 {"gpt2",           SplitPattern::GPT2},
                                                                                 {"r50k_base",      SplitPattern::R50K},
                                                                                 {"p50k_base",      SplitPattern::P50K},
                                                                                 {"p50k_edit",      SplitPattern::P50KEdit},
                                                                                 {"cl100k_base",    SplitPattern::CL100K},
                                                                                 {"o200k_base",     SplitPattern::O200K},
                                                                                 {"o200k_harmony",  SplitPattern::O200KHarmony},

                                                                                 {"gpt4",           SplitPattern::GPT4},

                                                                                 {"llama-v3",       SplitPattern::LLaMA3},
                                                                                 {"llama3",         SplitPattern::LLaMA3},

                                                                                 {"qwen2",          SplitPattern::Qwen2},
                                                                                 {"qwen2.5",        SplitPattern::Qwen2},
                                                                                 {"qwen3",          SplitPattern::Qwen2},
                                                                                 }};

    [[nodiscard]] static constexpr SplitPattern mapPreTokenizer(
        std::string_view name) noexcept
    {
        for (const auto &entry : kPreTokenizerMap)
            if (entry.name == name)
                return entry.pattern;

        return SplitPattern::None;
    }


    [[nodiscard]] static constexpr TokenType mapGgufModelToType(std::string_view model) noexcept
    {
        if (containsName(kBpeNames, model))
            return TokenType::BPE;

        if (containsName(kWordPieceNames, model))
            return TokenType::WordPiece;

        if (containsName(kUnigramNames, model))
            return TokenType::Unigram;

        return TokenType::Unknown;
    }

    [[nodiscard]] static constexpr StructuralType mapTokenType(GgufTokenType type) noexcept
    {
        switch (type) {
        case GgufTokenType::Normal:
            return StructuralType::Normal;

        case GgufTokenType::Unknown:
            return StructuralType::Unknown;

        case GgufTokenType::Control:
            return StructuralType::Control;

        case GgufTokenType::UserDefined:
            return StructuralType::UserDefined;

        case GgufTokenType::Unused:
            return StructuralType::Unused;

        case GgufTokenType::Byte:
            return StructuralType::Byte;

        default:
            return StructuralType::Normal;
        }
    }

private:
    ggml::JobGguf::UPtr m_gguf;

    std::string m_modelName;
    std::string m_preTokenizer;

    // UNRESOLVED:
    // HF and GGUF both expose BPE merges. Their provider priority and
    // normalization into runtime BPE rules will be handled by the facade.
    Merges      m_merges;

    inline static constexpr std::array<std::string_view, 23> kBpeNames = {
        "gpt2",
        "llama",
        "falcon",
        "qwen",
        "qwen2",
        "qwen3",
        "command-r",
        "mpt",
        "starcoder",
        "refact",
        "deepseek",
        "deepseek2",
        "deepseek3",
        "internlm2",
        "phi3",
        "phi4",
        "exaone",
        "chameleon",
        "smollm",
        "olmo",
        "olmo2",
        "nemotron",
        "minicpm"
    };

    inline static constexpr std::array<std::string_view, 2> kWordPieceNames = {
        "bert",
        "nomic-bert"
    };

    inline static constexpr std::array<std::string_view, 6> kUnigramNames = {
        "t5",
        "gemma",
        "gemma2",
        "gemma3",
        "gemma4",
        "spm"
    };
};

} // namespace job::token