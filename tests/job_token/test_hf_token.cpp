#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include <job_token_enums.h>
#include <job_token_types.h>
#include <token/hf_token.h>
#include <vocab/vocab.h>

#include "../transient_test_file.h"

using job::token::HfToken;
using job::token::IToken;
using job::token::SplitPattern;
using job::token::TokenId;
using job::token::TokenType;
using job::token::kInvalidToken;

//
// Block 1: usage / examples
//

static std::filesystem::path hfDataPath(std::string_view relativePath)
{
    return std::filesystem::path{JOB_TOKEN_TEST_DATA_DIR} / std::filesystem::path{relativePath};
}

TEST_CASE("HfToken loads checked-in Gemma tokenizer fixture", "[token][hf][integration][gemma]")
{
    HfToken token;
    const std::filesystem::path tokenizerPath = hfDataPath("gemma-4-12b-it/tokenizer.json");
    const std::filesystem::path configPath = hfDataPath("gemma-4-12b-it/tokenizer_config.json");

    REQUIRE(token.load(tokenizerPath, configPath));
    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() != TokenType::Unknown);
    REQUIRE(token.vocabSize() > 0);
}

TEST_CASE("HfToken loads checked-in Qwen tokenizer fixture", "[token][hf][integration][qwen]")
{
    HfToken token;
    const std::filesystem::path tokenizerPath = hfDataPath("Qwen3.8-27B/tokenizer.json");
    const std::filesystem::path configPath = hfDataPath("Qwen3.8-27B/tokenizer_config.json");

    REQUIRE(token.load(tokenizerPath, configPath));
    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() != TokenType::Unknown);
    REQUIRE(token.vocabSize() > 0);
}

TEST_CASE("HfToken starts as HuggingFace provider", "[token][hf][usage]")
{
    HfToken token;

    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.vocab() != nullptr);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
    REQUIRE(token.cleanUpTokenizationSpaces());
}

TEST_CASE("HfToken loads BPE vocabulary from tokenizer JSON", "[token][hf][usage][json][bpe]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "a": 0,
                "b": 1,
                "ab": 2
            },
            "merges": []
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() == TokenType::BPE);
    REQUIRE(token.vocabSize() == 3);
    REQUIRE(token.vocab()->findId("a") == 0);
    REQUIRE(token.vocab()->findId("b") == 1);
    REQUIRE(token.vocab()->findId("ab") == 2);
    REQUIRE(token.vocab()->tokenText(0) == "a");
    REQUIRE(token.vocab()->tokenText(1) == "b");
    REQUIRE(token.vocab()->tokenText(2) == "ab");
}

TEST_CASE("HfToken preserves sparse vocabulary IDs", "[token][hf][usage][json][vocab]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "zero": 0,
                "three": 3
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.vocabSize() == 4);
    REQUIRE(token.vocab()->record(0) != nullptr);
    REQUIRE(token.vocab()->record(1) == nullptr);
    REQUIRE(token.vocab()->record(2) == nullptr);
    REQUIRE(token.vocab()->record(3) != nullptr);
    REQUIRE(token.vocab()->findId("zero") == 0);
    REQUIRE(token.vocab()->findId("three") == 3);
}

TEST_CASE("HfToken loads Unigram vocabulary and scores from array form", "[token][hf][usage][json][unigram]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "Unigram",
            "vocab": [
                ["<unk>", 0.0],
                ["hello", -1.25],
                ["world", -2.5]
            ]
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.tokenType() == TokenType::Unigram);
    REQUIRE(token.vocabSize() == 3);
    REQUIRE(token.vocab()->tokenText(0) == "<unk>");
    REQUIRE(token.vocab()->tokenText(1) == "hello");
    REQUIRE(token.vocab()->tokenText(2) == "world");
    REQUIRE(token.vocab()->tokenScore(0) == 0.0f);
    REQUIRE(token.vocab()->tokenScore(1) == -1.25f);
    REQUIRE(token.vocab()->tokenScore(2) == -2.5f);
}

TEST_CASE("HfToken loads WordPiece model type", "[token][hf][usage][json][wordpiece]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "WordPiece",
            "vocab": {
                "[UNK]": 0,
                "play": 1,
                "##ing": 2
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.tokenType() == TokenType::WordPiece);
    REQUIRE(token.vocabSize() == 3);
    REQUIRE(token.vocab()->findId("play") == 1);
    REQUIRE(token.vocab()->findId("##ing") == 2);
}

TEST_CASE("HfToken loads BPE merges from string form", "[token][hf][usage][json][merges]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "a": 0,
                "b": 1,
                "c": 2
            },
            "merges": [
                "a b",
                "ab c"
            ]
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.merges().size() == 2);
    REQUIRE(token.merges()[0].first == "a");
    REQUIRE(token.merges()[0].second == "b");
    REQUIRE(token.merges()[1].first == "ab");
    REQUIRE(token.merges()[1].second == "c");
}

TEST_CASE("HfToken loads BPE merges from pair-array form", "[token][hf][usage][json][merges]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "a": 0,
                "b": 1,
                "c": 2
            },
            "merges": [
                ["a", "b"],
                ["ab", "c"]
            ]
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.merges().size() == 2);
    REQUIRE(token.merges()[0].first == "a");
    REQUIRE(token.merges()[0].second == "b");
    REQUIRE(token.merges()[1].first == "ab");
    REQUIRE(token.merges()[1].second == "c");
}

TEST_CASE("HfToken reconciles added tokens into canonical vocabulary", "[token][hf][usage][json][added-token]")
{
    static constexpr std::string_view Json = R"({
        "added_tokens": [
            {
                "id": 3,
                "content": "<special>",
                "special": true,
                "single_word": false,
                "lstrip": false,
                "rstrip": false,
                "normalized": false
            }
        ],
        "model": {
            "type": "BPE",
            "vocab": {
                "a": 0,
                "b": 1
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.vocabSize() == 4);
    REQUIRE(token.vocab()->record(2) == nullptr);
    REQUIRE(token.vocab()->record(3) != nullptr);
    REQUIRE(token.vocab()->findId("<special>") == 3);
    REQUIRE(token.vocab()->tokenText(3) == "<special>");
}

TEST_CASE("HfToken resolves model unknown token into SpecialTokens", "[token][hf][usage][json][special]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "WordPiece",
            "unk_token": "[UNK]",
            "vocab": {
                "[UNK]": 0,
                "hello": 1
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.specialTokens() != nullptr);
    REQUIRE(token.specialTokens()->unkId() == 0);
    REQUIRE(token.specialTokens()->isSpecial(0));
}

TEST_CASE("HfToken reads pre-tokenizer flags from tokenizer JSON", "[token][hf][usage][json][pretokenizer]")
{
    static constexpr std::string_view Json = R"({
        "pre_tokenizer": {
            "type": "ByteLevel",
            "byte_fallback": true,
            "add_prefix_space": true
        },
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.byteFallback());
    REQUIRE(token.addPrefixSpace());
}

TEST_CASE("HfToken model byte fallback overrides pre-tokenizer byte fallback", "[token][hf][usage][json][pretokenizer]")
{
    static constexpr std::string_view Json = R"({
        "pre_tokenizer": {
            "byte_fallback": true
        },
        "model": {
            "type": "BPE",
            "byte_fallback": false,
            "vocab": {
                "hello": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE_FALSE(token.byteFallback());
}

TEST_CASE("HfToken loads string chat template from tokenizer JSON", "[token][hf][usage][json][chat]")
{
    static constexpr std::string_view Json = R"({
        "chat_template": "{{ messages }}",
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.chatTemplate() == "{{ messages }}");
}

TEST_CASE("HfToken chooses default chat template from tokenizer JSON array", "[token][hf][usage][json][chat]")
{
    static constexpr std::string_view Json = R"({
        "chat_template": [
            {
                "name": "tool_use",
                "template": "TOOL"
            },
            {
                "name": "default",
                "template": "DEFAULT"
            },
            {
                "name": "other",
                "template": "OTHER"
            }
        ],
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.chatTemplate() == "DEFAULT");
}

TEST_CASE("HfToken config resolves canonical special token IDs", "[token][hf][usage][config][special]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "<bos>": 0,
                "<eos>": 1,
                "<unk>": 2,
                "<pad>": 3,
                "<cls>": 4,
                "<sep>": 5,
                "<mask>": 6,
                "hello": 7
            }
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "bos_token": "<bos>",
        "eos_token": "<eos>",
        "unk_token": "<unk>",
        "pad_token": "<pad>",
        "cls_token": "<cls>",
        "sep_token": "<sep>",
        "mask_token": "<mask>"
    })";

    HfToken token;

    REQUIRE(token.load(TokenizerJson, ConfigJson));
    REQUIRE(token.specialTokens()->bosId() == 0);
    REQUIRE(token.specialTokens()->eosId() == 1);
    REQUIRE(token.specialTokens()->unkId() == 2);
    REQUIRE(token.specialTokens()->padId() == 3);
    REQUIRE(token.specialTokens()->clsId() == 4);
    REQUIRE(token.specialTokens()->sepId() == 5);
    REQUIRE(token.specialTokens()->maskId() == 6);
    REQUIRE(token.specialTokens()->isSpecial(0));
    REQUIRE(token.specialTokens()->isSpecial(1));
    REQUIRE(token.specialTokens()->isSpecial(2));
    REQUIRE(token.specialTokens()->isSpecial(3));
    REQUIRE(token.specialTokens()->isSpecial(4));
    REQUIRE(token.specialTokens()->isSpecial(5));
    REQUIRE(token.specialTokens()->isSpecial(6));
}

TEST_CASE("HfToken config accepts object-form special tokens", "[token][hf][usage][config][special]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "<bos>": 0,
                "<eos>": 1,
                "hello": 2
            }
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "bos_token": {
            "content": "<bos>"
        },
        "eos_token": {
            "content": "<eos>"
        }
    })";

    HfToken token;

    REQUIRE(token.load(TokenizerJson, ConfigJson));
    REQUIRE(token.specialTokens()->bosId() == 0);
    REQUIRE(token.specialTokens()->eosId() == 1);
}

TEST_CASE("HfToken config loads sequence behavior and cleanup policy", "[token][hf][usage][config][sequence]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "add_bos_token": true,
        "add_eos_token": true,
        "clean_up_tokenization_spaces": false
    })";

    HfToken token;

    REQUIRE(token.load(TokenizerJson, ConfigJson));
    REQUIRE(token.addBosToken());
    REQUIRE(token.addEosToken());
    REQUIRE_FALSE(token.cleanUpTokenizationSpaces());
}

TEST_CASE("HfToken config chat template overrides tokenizer chat template", "[token][hf][usage][config][chat]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "chat_template": "TOKENIZER",
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "chat_template": "CONFIG"
    })";

    HfToken token;

    REQUIRE(token.load(TokenizerJson, ConfigJson));
    REQUIRE(token.chatTemplate() == "CONFIG");
}

TEST_CASE("HfToken loadConfig applies configuration after tokenizer load", "[token][hf][usage][io][config]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "<bos>": 0,
                "hello": 1
            }
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "bos_token": "<bos>",
        "add_bos_token": true,
        "clean_up_tokenization_spaces": false
    })";

    TransientTestFile configFile{"test_hf_token_config.json", ConfigJson};

    HfToken token;

    REQUIRE(token.loadJson(TokenizerJson));
    REQUIRE(token.loadConfig(configFile.path()));
    REQUIRE(token.specialTokens()->bosId() == 0);
    REQUIRE(token.addBosToken());
    REQUIRE_FALSE(token.cleanUpTokenizationSpaces());
}

TEST_CASE("HfToken loads tokenizer and config from files", "[token][hf][usage][io]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "<eos>": 0,
                "hello": 1
            },
            "merges": [
                "h e"
            ]
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "eos_token": "<eos>",
        "add_eos_token": true,
        "clean_up_tokenization_spaces": false
    })";

    TransientTestFile tokenizerFile{"test_hf_token_tokenizer.json", TokenizerJson};
    TransientTestFile configFile{"test_hf_token_tokenizer_config.json", ConfigJson};

    HfToken token;

    REQUIRE(token.load(std::filesystem::path{tokenizerFile.path()}, std::filesystem::path{configFile.path()}));
    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() == TokenType::BPE);
    REQUIRE(token.vocab()->findId("<eos>") == 0);
    REQUIRE(token.vocab()->findId("hello") == 1);
    REQUIRE(token.merges().size() == 1);
    REQUIRE(token.merges()[0].first == "h");
    REQUIRE(token.merges()[0].second == "e");
    REQUIRE(token.specialTokens()->eosId() == 0);
    REQUIRE(token.addEosToken());
    REQUIRE_FALSE(token.cleanUpTokenizationSpaces());
}

TEST_CASE("HfToken public lookup shortcuts expose loaded vocabulary", "[token][hf][usage][lookup]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 4
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));

    const auto id = token.findTokenId("hello");

    REQUIRE(id.has_value());
    REQUIRE(*id == 4);

    const auto text = token.findTokenString(4);

    REQUIRE(text.has_value());
    REQUIRE(*text == "hello");
}

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("HfToken rejects malformed tokenizer JSON", "[token][hf][edge][json]")
{
    HfToken token;

    REQUIRE_FALSE(token.loadJson("{ definitely not json"));
    REQUIRE(token.vocabSize() == 0);
}

TEST_CASE("HfToken rejects non-object tokenizer JSON", "[token][hf][edge][json]")
{
    HfToken token;

    REQUIRE_FALSE(token.loadJson("[]"));
    REQUIRE_FALSE(token.loadJson("\"tokenizer\""));
    REQUIRE_FALSE(token.loadJson("42"));
}

TEST_CASE("HfToken requires model object", "[token][hf][edge][json]")
{
    static constexpr std::string_view Json = R"({
        "added_tokens": []
    })";

    HfToken token;

    REQUIRE_FALSE(token.loadJson(Json));
}

TEST_CASE("HfToken requires a non-empty canonical vocabulary", "[token][hf][edge][json][vocab]")
{
    static constexpr std::string_view MissingVocab = R"({
        "model": {
            "type": "BPE"
        }
    })";
    static constexpr std::string_view EmptyVocab = R"({
        "model": {
            "type": "BPE",
            "vocab": {}
        }
    })";

    HfToken token;

    REQUIRE_FALSE(token.loadJson(MissingVocab));
    REQUIRE_FALSE(token.loadJson(EmptyVocab));
}

TEST_CASE("HfToken ignores invalid object vocabulary entries", "[token][hf][edge][json][vocab]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "good": 0,
                "negative": -1,
                "string": "2",
                "floating": 3.5
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.vocabSize() == 1);
    REQUIRE(token.vocab()->findId("good") == 0);
    REQUIRE(token.vocab()->findId("negative") == kInvalidToken);
    REQUIRE(token.vocab()->findId("string") == kInvalidToken);
    REQUIRE(token.vocab()->findId("floating") == kInvalidToken);
}

TEST_CASE("HfToken ignores malformed Unigram vocabulary entries", "[token][hf][edge][json][unigram]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "Unigram",
            "vocab": [
                ["good", -1.0],
                [],
                [42, -2.0],
                ["also-good"]
            ]
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.vocabSize() == 2);
    REQUIRE(token.vocab()->tokenText(0) == "good");
    REQUIRE(token.vocab()->tokenScore(0) == -1.0f);
    REQUIRE(token.vocab()->tokenText(1) == "also-good");
    REQUIRE(token.vocab()->tokenScore(1) == 0.0f);
}

TEST_CASE("HfToken ignores malformed merge entries", "[token][hf][edge][json][merges]")
{
    static constexpr std::string_view Json = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "a": 0
            },
            "merges": [
                "a b",
                "invalid",
                ["c", "d"],
                ["too", "many", "items"],
                [1, 2],
                42
            ]
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.merges().size() == 2);
    REQUIRE(token.merges()[0].first == "a");
    REQUIRE(token.merges()[0].second == "b");
    REQUIRE(token.merges()[1].first == "c");
    REQUIRE(token.merges()[1].second == "d");
}

TEST_CASE("HfToken ignores invalid added tokens", "[token][hf][edge][json][added-token]")
{
    static constexpr std::string_view Json = R"({
        "added_tokens": [
            {
                "id": -1,
                "content": "<negative>"
            },
            {
                "id": 4,
                "content": ""
            },
            {
                "content": "<missing-id>"
            },
            42
        ],
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.loadJson(Json));
    REQUIRE(token.vocabSize() == 1);
    REQUIRE(token.vocab()->findId("hello") == 0);
    REQUIRE(token.vocab()->findId("<negative>") == kInvalidToken);
    REQUIRE(token.vocab()->findId("<missing-id>") == kInvalidToken);
}

TEST_CASE("HfToken config ignores special tokens missing from vocabulary", "[token][hf][edge][config][special]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";
    static constexpr std::string_view ConfigJson = R"({
        "bos_token": "<bos>",
        "eos_token": "<eos>"
    })";

    HfToken token;

    REQUIRE(token.load(TokenizerJson, ConfigJson));
    REQUIRE(token.specialTokens()->bosId() == kInvalidToken);
    REQUIRE(token.specialTokens()->eosId() == kInvalidToken);
}

TEST_CASE("HfToken rejects malformed config JSON", "[token][hf][edge][config]")
{
    HfToken token;

    REQUIRE_FALSE(token.loadConfigJson("{ nope"));
}

TEST_CASE("HfToken loadConfig rejects missing file", "[token][hf][edge][io][config]")
{
    HfToken token;

    REQUIRE_FALSE(token.loadConfig("this_hf_config_should_not_exist.json"));
}

TEST_CASE("HfToken load rejects missing tokenizer file", "[token][hf][edge][io]")
{
    HfToken token;

    REQUIRE_FALSE(token.load(std::filesystem::path{"lenny_dykstra_I_mean_edsger_dijkstra_is_so_damn_greedy.json"}));
}

TEST_CASE("HfToken buffer load clears previous state before loading", "[token][hf][edge][state]")
{
    static constexpr std::string_view First = R"({
        "chat_template": "FIRST",
        "model": {
            "type": "BPE",
            "byte_fallback": true,
            "vocab": {
                "first": 0
            },
            "merges": [
                "f i"
            ]
        }
    })";
    static constexpr std::string_view Second = R"({
        "model": {
            "type": "WordPiece",
            "vocab": {
                "second": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.load(First));
    REQUIRE(token.tokenType() == TokenType::BPE);
    REQUIRE(token.byteFallback());
    REQUIRE(token.chatTemplate() == "FIRST");
    REQUIRE(token.merges().size() == 1);
    REQUIRE(token.vocab()->findId("first") == 0);

    REQUIRE(token.load(Second));
    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() == TokenType::WordPiece);
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE(token.chatTemplate().empty());
    REQUIRE(token.merges().empty());
    REQUIRE(token.vocabSize() == 1);
    REQUIRE(token.vocab()->findId("first") == kInvalidToken);
    REQUIRE(token.vocab()->findId("second") == 0);
    REQUIRE(token.cleanUpTokenizationSpaces());
}

TEST_CASE("HfToken clear restores HuggingFace defaults", "[token][hf][edge][state]")
{
    static constexpr std::string_view Json = R"({
        "chat_template": "CHAT",
        "model": {
            "type": "BPE",
            "byte_fallback": true,
            "vocab": {
                "hello": 0
            },
            "merges": [
                "h e"
            ]
        }
    })";
    static constexpr std::string_view Config = R"({
        "add_bos_token": true,
        "add_eos_token": true,
        "clean_up_tokenization_spaces": false
    })";

    HfToken token;

    REQUIRE(token.load(Json, Config));

    token.setSplitPattern(SplitPattern::GPT2);
    token.setAddPrefixSpace(true);
    token.clear();

    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
    REQUIRE(token.splitPattern() == SplitPattern::None);
    REQUIRE(token.customSplitPattern().empty());
    REQUIRE_FALSE(token.addPrefixSpace());
    REQUIRE_FALSE(token.byteFallback());
    REQUIRE_FALSE(token.addBosToken());
    REQUIRE_FALSE(token.addEosToken());
    REQUIRE(token.chatTemplate().empty());
    REQUIRE(token.cleanUpTokenizationSpaces());
}

TEST_CASE("HfToken failed buffer load leaves cleared common state", "[token][hf][edge][state][failure]")
{
    static constexpr std::string_view Valid = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    HfToken token;

    REQUIRE(token.load(Valid));
    REQUIRE(token.vocabSize() == 1);

    REQUIRE_FALSE(token.load(std::string_view{"{ malformed json"}));

    REQUIRE(token.provider() == IToken::Provider::HuggingFace);
    REQUIRE(token.tokenType() == TokenType::Unknown);
    REQUIRE(token.vocabSize() == 0);
    REQUIRE(token.merges().empty());
}

TEST_CASE("HfToken file load ignores missing optional config path", "[token][hf][edge][io][config]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    TransientTestFile tokenizerFile{"test_hf_token_optional_config_tokenizer.json", TokenizerJson};

    HfToken token;

    REQUIRE(token.load(std::filesystem::path{tokenizerFile.path()}, std::filesystem::path{"this_optional_hf_config_should_not_exist.json"}));
    REQUIRE(token.vocab()->findId("hello") == 0);
}

TEST_CASE("HfToken file load rejects malformed existing config", "[token][hf][edge][io][config]")
{
    static constexpr std::string_view TokenizerJson = R"({
        "model": {
            "type": "BPE",
            "vocab": {
                "hello": 0
            }
        }
    })";

    TransientTestFile tokenizerFile{"test_hf_token_bad_config_tokenizer.json", TokenizerJson};
    TransientTestFile configFile{"test_hf_token_bad_config.json", "{ malformed"};

    HfToken token;

    REQUIRE_FALSE(token.load(std::filesystem::path{tokenizerFile.path()}, std::filesystem::path{configFile.path()}));
}