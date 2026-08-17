#include "formats/hf_tokenizer_reader.h"
#include "formats/format_utils.h"

#include <job_logger.h>
#include <nlohmann/json.hpp>

namespace job::token {

bool HfTokenizerReader::loadFromFile(
    const std::filesystem::path& tokenizerJsonPath,
    const std::filesystem::path& tokenizerConfigJsonPath)
{
    clear();

    if (!std::filesystem::exists(tokenizerJsonPath)) {
        JOB_LOG_ERROR("Tokenizer JSON file does not exist: {}", tokenizerJsonPath.string());
        return false;
    }

    std::string tokenizerContent = utils::readFileToString(tokenizerJsonPath);
    if (tokenizerContent.empty() || !loadTokenizerJson(tokenizerContent)) {
        JOB_LOG_ERROR("Failed to read or parse tokenizer JSON: {}", tokenizerJsonPath.string());
        return false;
    }

    if (!tokenizerConfigJsonPath.empty() && std::filesystem::exists(tokenizerConfigJsonPath)) {
        std::string configContent = utils::readFileToString(tokenizerConfigJsonPath);
        if (!configContent.empty()) {
            if (!loadTokenizerConfigJson(configContent)) {
                JOB_LOG_WARN("Failed to parse tokenizer config JSON: {}", tokenizerConfigJsonPath.string());
                return false;
            }
        }
    }

    // If chat_template was not in the JSONs, check for sibling chat_template.jinja
    if (m_data.chatTemplate.empty()) {
        auto jinjaPath = tokenizerJsonPath.parent_path() / "chat_template.jinja";
        if (std::filesystem::exists(jinjaPath)) {
            m_data.chatTemplate = utils::readFileToString(jinjaPath);
        }
    }

    return true;
}

bool HfTokenizerReader::loadFromMemory(
    std::string_view tokenizerJson,
    std::string_view tokenizerConfigJson)
{
    clear();

    if (!loadTokenizerJson(tokenizerJson)) {
        return false;
    }

    if (!tokenizerConfigJson.empty()) {
        if (!loadTokenizerConfigJson(tokenizerConfigJson)) {
            JOB_LOG_WARN("Failed to parse tokenizer config JSON buffer in loadFromMemory");
            return false;
        }
    }

    return true;
}

bool HfTokenizerReader::loadTokenizerJson(std::string_view jsonContent)
{
    nlohmann::json root = nlohmann::json::parse(jsonContent, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return false;
    }

    // 1. Added Tokens List
    if (root.contains("added_tokens") && root["added_tokens"].is_array()) {
        for (const auto& item : root["added_tokens"]) {
            if (!item.is_object()) continue;

            HfAddedToken tok;
            if (item.contains("id") && item["id"].is_number_integer()) {
                tok.id = item["id"].get<int32_t>();
            }
            if (item.contains("content") && item["content"].is_string()) {
                tok.content = item["content"].get<std::string>();
            }
            if (item.contains("special") && item["special"].is_boolean()) {
                tok.special = item["special"].get<bool>();
            }
            if (item.contains("single_word") && item["single_word"].is_boolean()) {
                tok.singleWord = item["single_word"].get<bool>();
            }
            if (item.contains("lstrip") && item["lstrip"].is_boolean()) {
                tok.lstrip = item["lstrip"].get<bool>();
            }
            if (item.contains("rstrip") && item["rstrip"].is_boolean()) {
                tok.rstrip = item["rstrip"].get<bool>();
            }
            if (item.contains("normalized") && item["normalized"].is_boolean()) {
                tok.normalized = item["normalized"].get<bool>();
            }

            if (tok.id >= 0 && !tok.content.empty()) {
                m_data.addedTokens.push_back(std::move(tok));
            }
        }
    }

    // 2. Chat template embedded in tokenizer.json (string or list of templates)
    if (root.contains("chat_template")) {
        const auto& ct = root["chat_template"];
        if (ct.is_string()) {
            m_data.chatTemplate = ct.get<std::string>();
        } else if (ct.is_array()) {
            for (const auto& item : ct) {
                if (item.is_object() && item.contains("template") && item["template"].is_string()) {
                    if ((item.contains("name") && item["name"] == "default") || m_data.chatTemplate.empty()) {
                        m_data.chatTemplate = item["template"].get<std::string>();
                    }
                }
            }
        }
    }

    // 3. Pre-tokenizer / normalizer settings
    if (root.contains("pre_tokenizer") && root["pre_tokenizer"].is_object()) {
        const auto& pret = root["pre_tokenizer"];
        if (pret.contains("byte_fallback") && pret["byte_fallback"].is_boolean()) {
            m_data.byteFallback = pret["byte_fallback"].get<bool>();
        }
        if (pret.contains("add_prefix_space") && pret["add_prefix_space"].is_boolean()) {
            m_data.addPrefixSpace = pret["add_prefix_space"].get<bool>();
        }
    }

    // 4. Model Object (Vocab & Merges)
    if (!root.contains("model") || !root["model"].is_object()) {
        return false;
    }

    const auto& model = root["model"];

    if (model.contains("type") && model["type"].is_string()) {
        m_data.modelType = stringToHfModelType(model["type"].get<std::string>());
    }

    if (model.contains("byte_fallback") && model["byte_fallback"].is_boolean()) {
        m_data.byteFallback = model["byte_fallback"].get<bool>();
    }
    if (model.contains("unk_token") && model["unk_token"].is_string()) {
        m_data.unkToken = model["unk_token"].get<std::string>();
    }

    // Parse Model Vocab
    if (model.contains("vocab")) {
        const auto& vocab = model["vocab"];
        if (vocab.is_object()) {
            m_data.vocab.resize(vocab.size());
            for (auto it = vocab.begin(); it != vocab.end(); ++it) {
                if (!it.value().is_number_integer()) continue;
                int64_t id = it.value().get<int64_t>();
                if (id >= 0) {
                    if (static_cast<size_t>(id) >= m_data.vocab.size()) {
                        m_data.vocab.resize(static_cast<size_t>(id + 1));
                    }
                    m_data.vocab[static_cast<size_t>(id)] = {it.key(), 0.0f};
                    m_data.tokenToId[it.key()] = static_cast<int32_t>(id);
                }
            }
        } else if (vocab.is_array()) {
            // Unigram model stores vocab as array of [token, score]
            m_data.vocab.reserve(vocab.size());
            int32_t id = 0;
            for (const auto& item : vocab) {
                if (item.is_array() && !item.empty() && item[0].is_string()) {
                    std::string tokenStr = item[0].get<std::string>();
                    float score = (item.size() > 1 && item[1].is_number()) ? item[1].get<float>() : 0.0f;
                    m_data.vocab.emplace_back(tokenStr, score);
                    m_data.tokenToId[tokenStr] = id++;
                }
            }
        }
    }

    // Parse BPE Merges
    /*
    if (model.contains("merges") && model["merges"].is_array()) {
        for (const auto& item : model["merges"]) {
            if (!item.is_string()) continue;
            std::string rule = item.get<std::string>();
            size_t spacePos = rule.find(' ');
            if (spacePos != std::string::npos) {
                m_data.merges.emplace_back(
                    rule.substr(0, spacePos),
                    rule.substr(spacePos + 1));
            }
        }
    }
    */

    // Parse BPE Merges
    if (model.contains("merges") && model["merges"].is_array()) {
        for (const auto &item : model["merges"]) {
            if (item.is_array() &&
                item.size() == 2 &&
                item[0].is_string() &&
                item[1].is_string()) {

                m_data.merges.emplace_back(
                    item[0].get<std::string>(),
                    item[1].get<std::string>());

                continue;
            }

            if (item.is_string()) {
                const std::string rule = item.get<std::string>();
                const std::size_t spacePos = rule.find(' ');

                if (spacePos != std::string::npos) {
                    m_data.merges.emplace_back(
                        rule.substr(0, spacePos),
                        rule.substr(spacePos + 1));
                }
            }
        }
    }





    // Reconcile added tokens into vocab mapping if missing
    for (const auto& tok : m_data.addedTokens) {
        if (tok.id >= 0) {
            if (static_cast<size_t>(tok.id) >= m_data.vocab.size()) {
                m_data.vocab.resize(static_cast<size_t>(tok.id + 1));
            }
            m_data.vocab[static_cast<size_t>(tok.id)] = {tok.content, 0.0f};
            m_data.tokenToId[tok.content] = tok.id;
        }
    }

    return !m_data.vocab.empty();
}

bool HfTokenizerReader::loadTokenizerConfigFile(const std::filesystem::path& configPath)
{
    std::string content = utils::readFileToString(configPath);
    if (content.empty()) return false;
    return loadTokenizerConfigJson(content);
}

bool HfTokenizerReader::loadTokenizerConfigJson(std::string_view jsonContent)
{
    nlohmann::json root = nlohmann::json::parse(jsonContent, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return false;
    }

    if (root.contains("chat_template")) {
        const auto& ct = root["chat_template"];
        if (ct.is_string()) {
            m_data.chatTemplate = ct.get<std::string>();
        } else if (ct.is_array()) {
            for (const auto& item : ct) {
                if (item.is_object() && item.contains("template") && item["template"].is_string()) {
                    if ((item.contains("name") && item["name"] == "default") || m_data.chatTemplate.empty()) {
                        m_data.chatTemplate = item["template"].get<std::string>();
                    }
                }
            }
        }
    }

    if (root.contains("bos_token"))  m_data.bosToken  = utils::extractTokenString(root["bos_token"]);
    if (root.contains("eos_token"))  m_data.eosToken  = utils::extractTokenString(root["eos_token"]);
    if (root.contains("unk_token"))  m_data.unkToken  = utils::extractTokenString(root["unk_token"]);
    if (root.contains("pad_token"))  m_data.padToken  = utils::extractTokenString(root["pad_token"]);
    if (root.contains("cls_token"))  m_data.clsToken  = utils::extractTokenString(root["cls_token"]);
    if (root.contains("sep_token"))  m_data.sepToken  = utils::extractTokenString(root["sep_token"]);
    if (root.contains("mask_token")) m_data.maskToken = utils::extractTokenString(root["mask_token"]);

    if (root.contains("add_bos_token") && root["add_bos_token"].is_boolean()) {
        m_data.addBosToken = root["add_bos_token"].get<bool>();
    }
    if (root.contains("add_eos_token") && root["add_eos_token"].is_boolean()) {
        m_data.addEosToken = root["add_eos_token"].get<bool>();
    }
    if (root.contains("clean_up_tokenization_spaces") && root["clean_up_tokenization_spaces"].is_boolean()) {
        m_data.cleanUpTokenizationSpaces = root["clean_up_tokenization_spaces"].get<bool>();
    }

    return true;
}

std::optional<int32_t> HfTokenizerReader::findTokenId(std::string_view token) const noexcept
{
    auto it = m_data.tokenToId.find(std::string(token));
    if (it != m_data.tokenToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string_view> HfTokenizerReader::findTokenString(int32_t id) const noexcept
{
    if (id >= 0 && static_cast<size_t>(id) < m_data.vocab.size()) {
        return m_data.vocab[static_cast<size_t>(id)].first;
    }
    return std::nullopt;
}

void HfTokenizerReader::clear() noexcept
{
    m_data = HfTokenizerData{};
}

} // namespace job::token