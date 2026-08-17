#include "token_factory.h"

#include "binary_token.h"
#include "gguf_token.h"
#include "hf_token.h"

#include <job_logger.h>

namespace job::token {

IToken::UPtr TokenFactory::create(IToken::Provider provider, const std::filesystem::path &modelPath)
{
    switch (provider) {
    case IToken::Provider::HuggingFace: {
        auto token = HfToken::createUniq();
        if (!token->load(modelPath, {})) {
            JOB_LOG_ERROR("Failed to load HuggingFace tokenizer from '{}'", modelPath.string());
            return {};
        }

        return token;
    }

    case IToken::Provider::Gguf: {
        auto token = GgufToken::createUniq();
        if (!token->load(modelPath)) {
            JOB_LOG_ERROR("Failed to load GGUF tokenizer from '{}'", modelPath.string());
            return {};
        }

        return token;
    }

    case IToken::Provider::Binary: {
        auto token = BinaryToken::createUniq();
        if (!token->load(modelPath)) {
            JOB_LOG_ERROR("Failed to load binary tokenizer from '{}'", modelPath.string());
            return {};
        }

        return token;
    }

    case IToken::Provider::Unknown:
    default:
        JOB_LOG_ERROR("Unable to create tokenizer: unknown provider");
        return {};
    }
}

IToken::UPtr TokenFactory::create(IToken::Provider provider,
                                  const std::filesystem::path &modelPath,
                                  const std::filesystem::path &tokenizerPath)
{
    switch (provider) {
    case IToken::Provider::HuggingFace: {
        auto token = HfToken::createUniq();
        // HfToken consumes tokenizer.json first and config.json second.
        if (!token->load(tokenizerPath, modelPath)) {
            JOB_LOG_ERROR("Failed to load HuggingFace tokenizer (Model: '{}', Tokenizer: '{}')", modelPath.string(), tokenizerPath.string());
            return {};
        }

        return token;
    }

    case IToken::Provider::Gguf: {
        auto token = GgufToken::createUniq();
        // GGUF embeds the tokenizer description in the model itself.
        if (!token->load(modelPath)) {
            JOB_LOG_ERROR("Failed to load GGUF tokenizer from '{}'", modelPath.string());
            return {};
        }

        return token;
    }

    case IToken::Provider::Binary: {
        auto token = BinaryToken::createUniq();
        // The explicit tokenizer path wins for the two-path overload.
        if (!token->load(tokenizerPath)) {
            JOB_LOG_ERROR("Failed to load binary tokenizer from '{}'", tokenizerPath.string());
            return {};
        }

        return token;
    }

    case IToken::Provider::Unknown:
    default:
        JOB_LOG_ERROR("Unable to create tokenizer: unknown provider");
        return {};
    }
}

} // namespace job::token