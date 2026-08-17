#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "algo/itoken_algo.h"

#include "chat/chat_engine.h"

#include "core/regex_splitter.h"
#include "core/unicode_normalizer.h"

#include "encoder/ibyte_encoder.h"

#include "token/itoken.h"



#include "algo/token_algo_factory.h"
#include "token/token_factory.h"

#include "job_token_types.h"
#include "jobtoken_export.h"

namespace job::token {
class JOBTOKEN_EXPORT JobToken
{
public:
    using Ptr  = std::shared_ptr<JobToken>;
    using WPtr = std::weak_ptr<JobToken>;
    using UPtr = std::unique_ptr<JobToken>;

    JobToken() = default;
    ~JobToken() = default;

    JobToken(const JobToken &) = delete;
    JobToken &operator=(const JobToken &) = delete;
    JobToken(JobToken &&) = delete;
    JobToken &operator=(JobToken &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobToken>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobToken>();
    }

    [[nodiscard]] bool load(IToken::Provider provider, const std::filesystem::path &modelPath);
    [[nodiscard]] bool load(IToken::Provider provider, const std::filesystem::path &modelPath, const std::filesystem::path &tokenizerPath);

    [[nodiscard]] bool setToken(IToken::UPtr token);
    [[nodiscard]] IToken *token() noexcept
    {
        return m_token.get();
    }

    [[nodiscard]] const IToken *token() const noexcept
    {
        return m_token.get();
    }

    [[nodiscard]] bool isReady() const noexcept
    {
        return m_token &&
               m_algorithm &&
               m_normalizer &&
               m_splitter;
    }

    [[nodiscard]] std::vector<TokenId> encode(std::string_view text) const;

    [[nodiscard]] std::string decode(std::span<const TokenId> tokens) const;

private:
    [[nodiscard]] bool configure();

    void clearRuntime() noexcept
    {
        m_chatEngine.reset();
        m_algorithm.reset();
        m_splitter.reset();
        m_normalizer.reset();
    }

    IToken::UPtr            m_token;
    UnicodeNormalizer::UPtr m_normalizer;
    RegexSplitter::UPtr     m_splitter;
    ITokenAlgo::UPtr        m_algorithm;
    ChatEngine::UPtr        m_chatEngine;
    IByteEncoder::UPtr      m_byteEncoder;
};

} // namespace job::token