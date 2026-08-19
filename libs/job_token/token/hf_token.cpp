#include "hf_token.h"

#include <fstream>
#include <sstream>
#include <utility>

#include <job_logger.h>


namespace job::token {

bool HfToken::load(const std::filesystem::path &jsonPath, const std::filesystem::path &configPath)
{
    clear();

    if (!std::filesystem::exists(jsonPath)) {
        JOB_LOG_ERROR("Tokenizer JSON file does not exist: {}", jsonPath.string());
        return false;
    }

    const std::string tokenizerContent = readFile(jsonPath);
    if (tokenizerContent.empty() || !loadJson(tokenizerContent)) {
        JOB_LOG_ERROR( "Failed to read or parse tokenizer JSON: {}", jsonPath.string());
        return false;
    }

    if (!configPath.empty() &&
        std::filesystem::exists(configPath)) {
        const std::string configContent = readFile(configPath);
        if (!configContent.empty() && !loadConfigJson(configContent)) {
            JOB_LOG_WARN("Failed to parse tokenizer config JSON: {}", configPath.string());
            return false;
        }
    }

    if (chatTemplate().empty()) {
        const std::filesystem::path jinjaPath = jsonPath.parent_path() / "chat_template.jinja";
        if (std::filesystem::exists(jinjaPath))
            setChatTemplate(readFile(jinjaPath));
    }

    return true;
}

bool HfToken::load(std::string_view json, std::string_view configJson)
{
    clear();

    if (!loadJson(json))
        return false;

    if (!configJson.empty() && !loadConfigJson(configJson)) {
        JOB_LOG_WARN("Failed to parse tokenizer config JSON buffer");
        return false;
    }

    return true;
}

#if 0
bool HfToken::loadJson(std::string_view json)
{
    const nlohmann::json root = nlohmann::json::parse(json,
                                                      nullptr,
                                                      false);

    if (root.is_discarded() || !root.is_object())
        return false;

    // Added tokens -----------------------------------------------------------
    if (root.contains("added_tokens") && root["added_tokens"].is_array()) {
        for (const auto &item : root["added_tokens"]) {
            if (!item.is_object())
                continue;

            HfAddedToken token;

            if (item.contains("id") && item["id"].is_number_integer())
                token.id = item["id"].get<TokenId>();

            if (item.contains("content") && item["content"].is_string())
                token.content = item["content"].get<std::string>();

            if (item.contains("special") && item["special"].is_boolean())
                 token.special = item["special"].get<bool>();

            if (item.contains("single_word") && item["single_word"].is_boolean())
                token.singleWord = item["single_word"].get<bool>();

            if (item.contains("lstrip") && item["lstrip"].is_boolean())
                 token.lstrip = item["lstrip"].get<bool>();

            if (item.contains("rstrip") && item["rstrip"].is_boolean())
                 token.rstrip = item["rstrip"].get<bool>();

            if (item.contains("normalized") && item["normalized"].is_boolean())
                token.normalized = item["normalized"].get<bool>();

            if (token.id != kInvalidToken && !token.content.empty())
                m_addedTokens.push_back(std::move(token));
        }
    }

    // Chat template ----------------------------------------------------------
    if (root.contains("chat_template")) {
        const auto &chat = root["chat_template"];

        if (chat.is_string()) {
            setChatTemplate(chat.get<std::string>());
        } else if (chat.is_array()) {
            for (const auto &item : chat) {
                if (!item.is_object() || !item.contains("template") || !item["template"].is_string())
                    continue;

                const bool isDefault = item.contains("name") && item["name"].is_string() && item["name"] == "default";
                if (isDefault || chatTemplate().empty())
                    setChatTemplate(item["template"].get<std::string>());

            }
        }
    }

    // Pre-tokenizer configuration -------------------------------------------
    if (root.contains("pre_tokenizer"))
        parsePreTokenizer(root["pre_tokenizer"]);

    // Model ------------------------------------------------------------------
    if (!root.contains("model") || !root["model"].is_object())
        return false;

    const auto &model = root["model"];
    if (model.contains("type") && model["type"].is_string())
        setTokenType(tokenTypeFromStr(model["type"].get<std::string>()));

    if (model.contains("byte_fallback") && model["byte_fallback"].is_boolean())
        setByteFallback(model["byte_fallback"].get<bool>());

    // Vocabulary -------------------------------------------------------------
    if (model.contains("vocab")) {
        const auto &modelVocab = model["vocab"];
        if (modelVocab.is_object()) {
            for (auto it = modelVocab.begin(); it != modelVocab.end(); ++it) {
                if (!it.value().is_number_integer())
                    continue;

                const std::int64_t rawId = it.value().get<std::int64_t>();
                if (rawId < 0)
                    continue;

                const TokenId id = static_cast<TokenId>(rawId);
                vocab()->setToken(id, it.key(), 0.0f);
            }
        } else if (modelVocab.is_array()) {
            // Unigram stores vocab as [token, score].
            TokenId id = 0;

            for (const auto &item : modelVocab) {
                if (!item.is_array() || item.empty() || !item[0].is_string())
                    continue;

                const std::string token = item[0].get<std::string>();
                const float score = item.size() > 1 &&
                                            item[1].is_number() ?
                                        item[1].get<float>() :
                                        0.0f;
                vocab()->setToken(id, token, score);
                ++id;
            }
        }
    }

    // BPE merges -------------------------------------------------------------
    if (model.contains("merges") && model["merges"].is_array()) {
        for (const auto &item : model["merges"]) {
            if (item.is_array() && item.size() == 2 && item[0].is_string() && item[1].is_string()) {
                merges().emplace_back(item[0].get<std::string>(), item[1].get<std::string>());
                continue;
            }

            if (!item.is_string())
                continue;

            const std::string rule = item.get<std::string>();
            const std::size_t separator = rule.find(' ');
            if (separator == std::string::npos)
                continue;

            merges().emplace_back(rule.substr(0, separator), rule.substr(separator + 1));
        }
    }

    // Reconcile added tokens into the canonical vocabulary -------------------

    for (const HfAddedToken &token : m_addedTokens) {
        if (token.id == kInvalidToken || token.content.empty())
            continue;

        vocab()->setToken(token.id, token.content, 0.0f);
    }

    // tokenizer.json sometimes supplies the unknown token directly
    // in the model object.
    for (const HfAddedToken &token : m_addedTokens) {
        if (token.id == kInvalidToken || token.content.empty())
            continue;

        const StructuralType type = token.special ?
                                        StructuralType::Control :
                                        StructuralType::Normal;
        vocab()->setToken(token.id, token.content, 0.0f, type);
        if (token.special)
            specialTokens()->registerSpecial(token.content, token.id);
    }

    return vocabSize() > 0;
}
#endif
bool HfToken::loadJson(std::string_view json)
{
    const nlohmann::json root =
        nlohmann::json::parse(
            json,
            nullptr,
            false);

    if (root.is_discarded() ||
        !root.is_object()) {
        return false;
    }

    // Added tokens -----------------------------------------------------------

    if (root.contains("added_tokens") &&
        root["added_tokens"].is_array()) {

        for (const auto &item : root["added_tokens"]) {
            if (!item.is_object())
                continue;

            HfAddedToken token;

            if (item.contains("id") &&
                item["id"].is_number_integer()) {
                token.id =
                    item["id"].get<TokenId>();
            }

            if (item.contains("content") &&
                item["content"].is_string()) {
                token.content =
                    item["content"].get<std::string>();
            }

            if (item.contains("special") &&
                item["special"].is_boolean()) {
                token.special =
                    item["special"].get<bool>();
            }

            if (item.contains("single_word") &&
                item["single_word"].is_boolean()) {
                token.singleWord =
                    item["single_word"].get<bool>();
            }

            if (item.contains("lstrip") &&
                item["lstrip"].is_boolean()) {
                token.lstrip =
                    item["lstrip"].get<bool>();
            }

            if (item.contains("rstrip") &&
                item["rstrip"].is_boolean()) {
                token.rstrip =
                    item["rstrip"].get<bool>();
            }

            if (item.contains("normalized") &&
                item["normalized"].is_boolean()) {
                token.normalized =
                    item["normalized"].get<bool>();
            }

            if (token.id != kInvalidToken &&
                !token.content.empty()) {
                m_addedTokens.push_back(
                    std::move(token));
            }
        }
    }

    // Chat template ----------------------------------------------------------

    if (root.contains("chat_template")) {
        const auto &chat =
            root["chat_template"];

        if (chat.is_string()) {
            setChatTemplate(
                chat.get<std::string>());
        } else if (chat.is_array()) {
            for (const auto &item : chat) {
                if (!item.is_object() ||
                    !item.contains("template") ||
                    !item["template"].is_string()) {
                    continue;
                }

                const bool isDefault =
                    item.contains("name") &&
                    item["name"].is_string() &&
                    item["name"] == "default";

                if (isDefault ||
                    chatTemplate().empty()) {
                    setChatTemplate(
                        item["template"].get<std::string>());
                }
            }
        }
    }

    // Pre-tokenizer configuration -------------------------------------------

    if (root.contains("pre_tokenizer"))
        parsePreTokenizer(root["pre_tokenizer"]);

    // Model ------------------------------------------------------------------

    if (!root.contains("model") ||
        !root["model"].is_object()) {
        return false;
    }

    const auto &model =
        root["model"];

    if (model.contains("type") &&
        model["type"].is_string()) {
        setTokenType(
            tokenTypeFromStr(
                model["type"].get<std::string>()));
    }

    if (model.contains("byte_fallback") &&
        model["byte_fallback"].is_boolean()) {
        setByteFallback(
            model["byte_fallback"].get<bool>());
    }

    // Vocabulary -------------------------------------------------------------

    if (model.contains("vocab")) {
        const auto &modelVocab =
            model["vocab"];

        if (modelVocab.is_object()) {
            for (auto it = modelVocab.begin();
                 it != modelVocab.end();
                 ++it) {

                if (!it.value().is_number_integer())
                    continue;

                const std::int64_t rawId =
                    it.value().get<std::int64_t>();

                if (rawId < 0)
                    continue;

                const TokenId id =
                    static_cast<TokenId>(
                        rawId);

                vocab()->setToken(
                    id,
                    it.key(),
                    0.0f);
            }
        } else if (modelVocab.is_array()) {
            // Unigram stores vocab as [token, score].
            TokenId id =
                0;

            for (const auto &item : modelVocab) {
                if (!item.is_array() ||
                    item.empty() ||
                    !item[0].is_string()) {
                    continue;
                }

                const std::string token =
                    item[0].get<std::string>();

                const float score =
                    item.size() > 1 &&
                            item[1].is_number()
                        ? item[1].get<float>()
                        : 0.0f;

                vocab()->setToken(
                    id,
                    token,
                    score);

                ++id;
            }
        }
    }

    // BPE merges -------------------------------------------------------------

    if (model.contains("merges") &&
        model["merges"].is_array()) {

        for (const auto &item : model["merges"]) {
            if (item.is_array() &&
                item.size() == 2 &&
                item[0].is_string() &&
                item[1].is_string()) {

                merges().emplace_back(
                    item[0].get<std::string>(),
                    item[1].get<std::string>());

                continue;
            }

            if (!item.is_string())
                continue;

            const std::string rule =
                item.get<std::string>();

            const std::size_t separator =
                rule.find(' ');

            if (separator ==
                std::string::npos) {
                continue;
            }

            merges().emplace_back(
                rule.substr(
                    0,
                    separator),
                rule.substr(
                    separator + 1));
        }
    }

    // Reconcile added tokens into the canonical vocabulary -------------------

    for (const HfAddedToken &token : m_addedTokens) {
        if (token.id == kInvalidToken ||
            token.content.empty()) {
            continue;
        }

        const StructuralType type =
            token.special
                ? StructuralType::Control
                : StructuralType::Normal;

        vocab()->setToken(
            token.id,
            token.content,
            0.0f,
            type);

        if (token.special) {
            specialTokens()->registerSpecial(
                token.content,
                token.id);
        }
    }

    // tokenizer.json sometimes supplies the unknown token directly
    // in the model object.

    if (model.contains("unk_token")) {
        const std::string token =
            tokenString(
                model["unk_token"]);

        if (!token.empty()) {
            const TokenId id =
                vocab()->findId(
                    token);

            if (id != kInvalidToken) {
                specialTokens()->setUnkId(
                    id);
            }
        }
    }

    return vocabSize() > 0;
}



bool HfToken::loadConfig(const std::filesystem::path &config)
{
    const std::string content = readFile(config);
    if (content.empty())
        return false;

    return loadConfigJson(content);
}

bool HfToken::loadConfigJson(std::string_view json)
{
    const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
    if (root.is_discarded() || !root.is_object())
        return false;

    // Chat template ----------------------------------------------------------

    if (root.contains("chat_template")) {
        const auto &chat = root["chat_template"];

        if (chat.is_string()) {
            setChatTemplate(chat.get<std::string>());
        } else if (chat.is_array()) {
            for (const auto &item : chat) {
                if (!item.is_object() || !item.contains("template") || !item["template"].is_string())
                    continue;

                const bool isDefault = item.contains("name") && item["name"].is_string() && item["name"] == "default";
                if (isDefault || chatTemplate().empty())
                    setChatTemplate( item["template"].get<std::string>());
            }
        }
    }

    // Special token identities ----------------------------------------------

    const auto setSpecialToken = [this, &root](std::string_view key, auto setter) {
        const std::string keyString{key};
        if (!root.contains(keyString))
            return;

        const std::string token = tokenString(root[keyString]);
        if (token.empty())
            return;

        const TokenId id = vocab()->findId(token);
        if (id == kInvalidToken)
            return;

        (specialTokens()->*setter)(id);
    };

    setSpecialToken("bos_token",    &SpecialTokens::setBosId);
    setSpecialToken("eos_token",    &SpecialTokens::setEosId);
    setSpecialToken("unk_token",    &SpecialTokens::setUnkId);
    setSpecialToken("pad_token",    &SpecialTokens::setPadId);
    setSpecialToken("cls_token",    &SpecialTokens::setClsId);
    setSpecialToken("sep_token",    &SpecialTokens::setSepId);
    setSpecialToken("mask_token",   &SpecialTokens::setMaskId);

    // Sequence configuration -------------------------------------------------

    if (root.contains("add_bos_token") && root["add_bos_token"].is_boolean())
        setAddBosToken( root["add_bos_token"].get<bool>());

    if (root.contains("add_eos_token") && root["add_eos_token"].is_boolean())
        setAddEosToken(root["add_eos_token"].get<bool>());

    if (root.contains("clean_up_tokenization_spaces") && root["clean_up_tokenization_spaces"].is_boolean())
        m_cleanUpTokenizationSpaces = root["clean_up_tokenization_spaces"].get<bool>();

    return true;
}

void HfToken::extraClear() noexcept
{
    setProvider(Provider::HuggingFace);
    m_addedTokens.clear();
    m_cleanUpTokenizationSpaces = true;
}


std::string HfToken::tokenString(const nlohmann::json &value)
{
    if (value.is_string())
        return value.get<std::string>();

    if (!value.is_object())
        return {};

    const auto it = value.find("content");
    if (it == value.end() || !it->is_string())
        return {};

    return it->get<std::string>();
}

void HfToken::parsePreTokenizer(const nlohmann::json &preTokenizer)
{
    if (!preTokenizer.is_object())
        return;

    if (preTokenizer.contains("type") && preTokenizer["type"].is_string()) {
        const std::string type = preTokenizer["type"].get<std::string>();
        if (type == "ByteLevel")
            setByteEncoding(ByteEncoding::Gpt2);
    }

    if (preTokenizer.contains("byte_fallback") && preTokenizer["byte_fallback"].is_boolean()) {
        setByteFallback(
            preTokenizer["byte_fallback"].get<bool>());
    }

    if (preTokenizer.contains("add_prefix_space") &&
        preTokenizer["add_prefix_space"].is_boolean()) {
        setAddPrefixSpace(preTokenizer["add_prefix_space"].get<bool>());
    }

    if (preTokenizer.contains("pretokenizers") && preTokenizer["pretokenizers"].is_array()){
        for (const auto &child : preTokenizer["pretokenizers"]){
            parsePreTokenizer(child);
        }
    }
}


} // namespace job::token