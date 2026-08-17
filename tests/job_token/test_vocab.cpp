#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include <vocab/vocab.h>

using job::token::kInvalidToken;
using job::token::StructuralType;
using job::token::TokenId;
using job::token::Vocab;

//
// Block 1: usage / examples
//

TEST_CASE("Vocab stores tokens and provides bidirectional token lookup", "[token][vocab][usage]")
{
    Vocab vocab;

    const TokenId helloId = vocab.addToken("hello", 1.25f, StructuralType::Normal);
    const TokenId worldId = vocab.addToken("world", 2.5f, StructuralType::Normal);

    REQUIRE(helloId == 0);
    REQUIRE(worldId == 1);

    REQUIRE(vocab.size() == 2);
    REQUIRE_FALSE(vocab.empty());

    REQUIRE(vocab.findId("hello") == helloId);
    REQUIRE(vocab.findId("world") == worldId);

    REQUIRE(vocab.tokenText(helloId) == "hello");
    REQUIRE(vocab.tokenText(worldId) == "world");

    REQUIRE(vocab.tokenScore(helloId) == 1.25f);
    REQUIRE(vocab.tokenScore(worldId) == 2.5f);

    REQUIRE(vocab.tokenType(helloId) == StructuralType::Normal);
    REQUIRE(vocab.tokenType(worldId) == StructuralType::Normal);
}

TEST_CASE("Vocab exposes canonical TokenRecord state", "[token][vocab][record][usage]")
{
    Vocab vocab;

    const TokenId id = vocab.addToken("<0x41>", -0.5f, StructuralType::Byte);

    const auto *record = vocab.record(id);
    REQUIRE(record != nullptr);
    REQUIRE(record->isValid());

    REQUIRE(record->id() == id);
    REQUIRE(record->text() == "<0x41>");
    REQUIRE(record->score() == -0.5f);
    REQUIRE(record->type() == StructuralType::Byte);

    REQUIRE(record->isByte());
    REQUIRE_FALSE(record->isSpecial());
    REQUIRE_FALSE(record->isUnused());
}

TEST_CASE("Vocab can update token score and structural type", "[token][vocab][mutation][usage]")
{
    Vocab vocab;
    const TokenId id = vocab.addToken("token");

    vocab.setTokenScore(id, 42.0f);
    vocab.setTokenType(id, StructuralType::Control);

    REQUIRE(vocab.tokenScore(id) == 42.0f);
    REQUIRE(vocab.tokenType(id) == StructuralType::Control);

    const auto *record = vocab.record(id);
    REQUIRE(record != nullptr);
    REQUIRE(record->isSpecial());
}

TEST_CASE("Vocab setToken can populate a known token ID", "[token][vocab][set][usage]")
{
    Vocab vocab;
    vocab.setToken(2, "third", 3.0f, StructuralType::UserDefined);
    REQUIRE(vocab.size() == 3);

    REQUIRE(vocab.findId("third") == 2);
    REQUIRE(vocab.tokenText(2) == "third");
    REQUIRE(vocab.tokenScore(2) == 3.0f);
    REQUIRE(vocab.tokenType(2) == StructuralType::UserDefined);

    REQUIRE(vocab.record(2) != nullptr);
    REQUIRE(vocab.record(2)->isSpecial());

    // setToken() may create holes while loading formats whose IDs are
    // authoritative. A resized vector is not the same thing as a valid token.
    REQUIRE(vocab.record(0) == nullptr);
    REQUIRE(vocab.record(1) == nullptr);
}

TEST_CASE("Vocab replacing a token removes its stale text lookup", "[token][vocab][lookup][usage]")
{
    Vocab vocab;
    vocab.setToken(0, "old-name", 1.0f);
    REQUIRE(vocab.findId("old-name") == 0);

    vocab.setToken(0, "new-name", 2.0f);
    REQUIRE(vocab.findId("old-name") == kInvalidToken);
    REQUIRE(vocab.findId("new-name") == 0);

    REQUIRE(vocab.tokenText(0) == "new-name");
    REQUIRE(vocab.tokenScore(0) == 2.0f);
}

TEST_CASE("Vocab records span provides mutable canonical token records", "[token][vocab][records][usage]")
{
    Vocab vocab;
    const TokenId first = vocab.addToken("first");
    const TokenId second = vocab.addToken("second");

    auto records = vocab.records();
    REQUIRE(records.size() == 2);

    records[static_cast<std::size_t>(first)].setScore(10.0f);
    records[static_cast<std::size_t>(second)].setType(StructuralType::Unused);

    REQUIRE(vocab.tokenScore(first) == 10.0f);
    REQUIRE(vocab.tokenType(second) == StructuralType::Unused);
}

TEST_CASE("Const Vocab exposes read-only records", "[token][vocab][records][const]")
{
    Vocab vocab;
    vocab.addToken("immutable-view", 7.0f, StructuralType::Normal);
    const Vocab &constVocab = vocab;

    const auto records = constVocab.records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].text() == "immutable-view");
    REQUIRE(records[0].score() == 7.0f);
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("Vocab rejects empty token text", "[token][vocab][edge]")
{
    Vocab vocab;
    REQUIRE(vocab.addToken("") == kInvalidToken);
    REQUIRE(vocab.empty());
    REQUIRE(vocab.size() == 0);

    vocab.setToken(5, "");
    REQUIRE(vocab.empty());
}

TEST_CASE("Vocab invalid IDs return safe defaults", "[token][vocab][edge]")
{
    Vocab vocab;
    vocab.addToken("valid", 1.0f, StructuralType::Control);
    REQUIRE(vocab.record(kInvalidToken) == nullptr);
    REQUIRE(vocab.record(1000) == nullptr);
    REQUIRE(vocab.tokenText(kInvalidToken).empty());
    REQUIRE(vocab.tokenText(1000).empty());
    REQUIRE(vocab.tokenScore(kInvalidToken) == 0.0f);
    REQUIRE(vocab.tokenScore(1000) == 0.0f);
    REQUIRE(vocab.tokenType(kInvalidToken) == StructuralType::Unknown);
    REQUIRE(vocab.tokenType(1000) == StructuralType::Unknown);
    REQUIRE(vocab.findId("does-not-exist") == kInvalidToken);
}

TEST_CASE("Vocab mutation helpers ignore invalid token IDs", "[token][vocab][edge][mutation]")
{
    Vocab vocab;
    const TokenId id = vocab.addToken("still-here", 5.0f, StructuralType::Normal);
    vocab.setTokenScore(kInvalidToken, 99.0f);
    vocab.setTokenScore(999, 99.0f);
    vocab.setTokenType(kInvalidToken, StructuralType::Byte);
    vocab.setTokenType(999, StructuralType::Byte);
    REQUIRE(vocab.tokenScore(id) == 5.0f);
    REQUIRE(vocab.tokenType(id) == StructuralType::Normal);
}

TEST_CASE("Vocab clear removes records lookups and special-token state", "[token][vocab][clear][edge]")
{
    Vocab vocab;
    const TokenId bosId = vocab.addToken("<s>", 0.0f, StructuralType::Control);
    vocab.addToken("hello");
    vocab.specialTokens().registerSpecial("bos_token",
                                          bosId,
                                          job::token::SpecialTokenType::Bos);

    REQUIRE(vocab.size() == 2);
    REQUIRE(vocab.findId("<s>") == bosId);
    REQUIRE(vocab.specialTokens().bosId() == bosId);
    REQUIRE(vocab.specialTokens().isSpecial(bosId));

    vocab.clear();

    REQUIRE(vocab.empty());
    REQUIRE(vocab.size() == 0);
    REQUIRE(vocab.findId("<s>") == kInvalidToken);
    REQUIRE(vocab.specialTokens().bosId() == kInvalidToken);
    REQUIRE_FALSE(vocab.specialTokens().isSpecial(bosId));
}

TEST_CASE("Vocab reserve changes capacity policy without changing logical contents", "[token][vocab][reserve][edge]")
{
    Vocab vocab;

    vocab.reserve(1024);
    REQUIRE(vocab.empty());
    REQUIRE(vocab.size() == 0);

    const TokenId id = vocab.addToken("after-reserve");
    REQUIRE(id == 0);
    REQUIRE(vocab.size() == 1);
    REQUIRE(vocab.findId("after-reserve") == id);
}