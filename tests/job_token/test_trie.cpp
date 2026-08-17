#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "trie.h"

using job::token::kInvalidToken;
using job::token::TokenId;
using job::token::Trie;

//
// Block 1: usage / examples
//

TEST_CASE("Trie stores and retrieves exact token strings", "[token][trie][usage]")
{
    Trie trie;
    trie.insert("the", 10);
    trie.insert("there", 20);
    trie.insert("their", 30);

    REQUIRE(trie.find("the") == 10);
    REQUIRE(trie.find("there") == 20);
    REQUIRE(trie.find("their") == 30);
    REQUIRE(trie.find("th") == kInvalidToken);
    REQUIRE(trie.find("them") == kInvalidToken);
}

TEST_CASE("Trie supports tokens that share common prefixes", "[token][trie][prefix][usage]")
{
    Trie trie;
    trie.insert("a", 1);
    trie.insert("ab", 2);
    trie.insert("abc", 3);
    trie.insert("abcd", 4);

    REQUIRE(trie.find("a") == 1);
    REQUIRE(trie.find("ab") == 2);
    REQUIRE(trie.find("abc") == 3);
    REQUIRE(trie.find("abcd") == 4);
}

TEST_CASE("Trie longestPrefix returns the longest token matching input start", "[token][trie][prefix][usage]")
{
    Trie trie;
    trie.insert("the", 10);
    trie.insert("there", 20);
    trie.insert("therefore", 30);

    const Trie::Match match = trie.longestPrefix("thereabouts");

    REQUIRE(match.id == 20);
    REQUIRE(match.length == 5);
}

TEST_CASE("Trie longestPrefix can match an entire input", "[token][trie][prefix][usage]")
{
    Trie trie;
    trie.insert("hello", 42);

    const Trie::Match match = trie.longestPrefix("hello");

    REQUIRE(match.id == 42);
    REQUIRE(match.length == 5);
}

TEST_CASE("Trie findAllPrefixes reports every token prefix in order", "[token][trie][prefix][usage]")
{
    Trie trie;
    trie.insert("a", 10);
    trie.insert("ab", 20);
    trie.insert("abc", 30);
    trie.insert("abcd", 40);

    std::vector<Trie::Match> matches;
    trie.findAllPrefixes("abcdef", matches);

    REQUIRE(matches.size() == 4);
    REQUIRE(matches[0].id == 10);
    REQUIRE(matches[0].length == 1);
    REQUIRE(matches[1].id == 20);
    REQUIRE(matches[1].length == 2);
    REQUIRE(matches[2].id == 30);
    REQUIRE(matches[2].length == 3);
    REQUIRE(matches[3].id == 40);
    REQUIRE(matches[3].length == 4);
}

TEST_CASE("Trie findAllPrefixes appends to caller-owned match buffer", "[token][trie][prefix][usage]")
{
    Trie trie;
    trie.insert("car", 100);
    trie.insert("cart", 200);

    std::vector<Trie::Match> matches{Trie::Match{999, 99}};
    trie.findAllPrefixes("cartwheel", matches);

    REQUIRE(matches.size() == 3);
    REQUIRE(matches[0].id == 999);
    REQUIRE(matches[0].length == 99);
    REQUIRE(matches[1].id == 100);
    REQUIRE(matches[1].length == 3);
    REQUIRE(matches[2].id == 200);
    REQUIRE(matches[2].length == 4);
}

TEST_CASE("Trie handles byte-oriented token strings", "[token][trie][bytes][usage]")
{
    Trie trie;
    const std::string token{static_cast<char>(0xC3), static_cast<char>(0xA9)};
    trie.insert(token, 77);

    REQUIRE(trie.find(token) == 77);

    const std::string text = token + "clair";
    const Trie::Match match = trie.longestPrefix(text);

    REQUIRE(match.id == 77);
    REQUIRE(match.length == token.size());
}

TEST_CASE("Trie reports node growth as tokens are inserted", "[token][trie][usage]")
{
    Trie trie;

    REQUIRE(trie.nodeCount() == 1);
    REQUIRE(trie.empty());

    trie.insert("abc", 1);

    // Root + a + b + c.
    REQUIRE(trie.nodeCount() == 4);
    REQUIRE_FALSE(trie.empty());

    // Shared prefix reuses a/b/c and only adds d.
    trie.insert("abcd", 2);

    REQUIRE(trie.nodeCount() == 5);
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("Empty Trie returns no matches", "[token][trie][edge]")
{
    Trie trie;

    REQUIRE(trie.empty());
    REQUIRE(trie.nodeCount() == 1);
    REQUIRE(trie.find("anything") == kInvalidToken);

    const Trie::Match match = trie.longestPrefix("anything");

    REQUIRE(match.id == kInvalidToken);
    REQUIRE(match.length == 0);

    std::vector<Trie::Match> matches;
    trie.findAllPrefixes("anything", matches);

    REQUIRE(matches.empty());
}

TEST_CASE("Trie ignores empty token insertion", "[token][trie][edge]")
{
    Trie trie;
    trie.insert("", 10);

    REQUIRE(trie.empty());
    REQUIRE(trie.nodeCount() == 1);
    REQUIRE(trie.find("") == kInvalidToken);
}

TEST_CASE("Trie ignores invalid token ID insertion", "[token][trie][edge]")
{
    Trie trie;
    trie.insert("invalid", kInvalidToken);

    REQUIRE(trie.empty());
    REQUIRE(trie.find("invalid") == kInvalidToken);
}

TEST_CASE("Trie distinguishes token prefixes from complete tokens", "[token][trie][edge][prefix]")
{
    Trie trie;
    trie.insert("testing", 5);

    REQUIRE(trie.find("test") == kInvalidToken);
    REQUIRE(trie.find("testing") == 5);

    const Trie::Match shortMatch = trie.longestPrefix("test");

    REQUIRE(shortMatch.id == kInvalidToken);
    REQUIRE(shortMatch.length == 0);
}

TEST_CASE("Trie longestPrefix returns no match when input diverges immediately", "[token][trie][edge][prefix]")
{
    Trie trie;
    trie.insert("apple", 10);
    trie.insert("banana", 20);

    const Trie::Match match = trie.longestPrefix("cranberry");

    REQUIRE(match.id == kInvalidToken);
    REQUIRE(match.length == 0);
}

TEST_CASE("Trie findAllPrefixes leaves caller buffer unchanged when nothing matches", "[token][trie][edge][prefix]")
{
    Trie trie;
    trie.insert("hello", 10);

    std::vector<Trie::Match> matches{Trie::Match{123, 4}};
    trie.findAllPrefixes("world", matches);

    REQUIRE(matches.size() == 1);
    REQUIRE(matches[0].id == 123);
    REQUIRE(matches[0].length == 4);
}

TEST_CASE("Trie clear restores fresh root state", "[token][trie][clear][edge]")
{
    Trie trie;
    trie.insert("one", 1);
    trie.insert("two", 2);
    trie.insert("three", 3);

    REQUIRE_FALSE(trie.empty());
    REQUIRE(trie.nodeCount() > 1);

    trie.clear();

    REQUIRE(trie.empty());
    REQUIRE(trie.nodeCount() == 1);
    REQUIRE(trie.find("one") == kInvalidToken);
    REQUIRE(trie.find("two") == kInvalidToken);
    REQUIRE(trie.find("three") == kInvalidToken);
}

TEST_CASE("Reinserting the same key updates its TokenId without growing Trie", "[token][trie][edge][mutation]")
{
    Trie trie;
    trie.insert("same", 10);

    const std::size_t nodeCount = trie.nodeCount();

    REQUIRE(trie.find("same") == 10);

    trie.insert("same", 20);

    REQUIRE(trie.find("same") == 20);
    REQUIRE(trie.nodeCount() == nodeCount);
}

TEST_CASE("Trie remains usable after clear and reuse", "[token][trie][clear][edge]")
{
    Trie trie;
    trie.insert("old", 10);
    trie.clear();
    trie.insert("new", 20);

    REQUIRE(trie.find("old") == kInvalidToken);
    REQUIRE(trie.find("new") == 20);
    REQUIRE_FALSE(trie.empty());
}