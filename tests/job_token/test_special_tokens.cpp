#include <catch2/catch_test_macros.hpp>

#include "special_tokens.h"

using job::token::kInvalidToken;
using job::token::SpecialTokenType;
using job::token::SpecialTokens;
using job::token::TokenId;

//
// Block 1: usage / examples
//

TEST_CASE("SpecialTokens stores canonical special token IDs", "[token][special][usage]")
{
    SpecialTokens special;
    special.setBosId(1);
    special.setEosId(2);
    special.setEotId(3);
    special.setPadId(4);
    special.setUnkId(5);
    special.setMaskId(6);
    special.setClsId(7);
    special.setSepId(8);
    special.setPrefixId(9);
    special.setSuffixId(10);
    special.setMiddleId(11);

    REQUIRE(special.bosId() == 1);
    REQUIRE(special.eosId() == 2);
    REQUIRE(special.eotId() == 3);
    REQUIRE(special.padId() == 4);
    REQUIRE(special.unkId() == 5);
    REQUIRE(special.maskId() == 6);
    REQUIRE(special.clsId() == 7);
    REQUIRE(special.sepId() == 8);
    REQUIRE(special.prefixId() == 9);
    REQUIRE(special.suffixId() == 10);
    REQUIRE(special.middleId() == 11);
}

TEST_CASE("Canonical IDs are automatically classified as special", "[token][special][usage]")
{
    SpecialTokens special;
    special.setBosId(10);
    special.setEosId(11);
    special.setPadId(12);

    REQUIRE(special.isSpecial(10));
    REQUIRE(special.isSpecial(11));
    REQUIRE(special.isSpecial(12));
    REQUIRE_FALSE(special.isSpecial(13));
}

TEST_CASE("registerSpecial creates bidirectional named lookup", "[token][special][lookup][usage]")
{
    SpecialTokens special;
    special.registerSpecial("bos_token", 42, SpecialTokenType::Bos);

    const auto id = special.findByName("bos_token");

    REQUIRE(id.has_value());
    REQUIRE(*id == 42);
    REQUIRE(special.nameById(42) == "bos_token");
    REQUIRE(special.bosId() == 42);
    REQUIRE(special.isSpecial(42));
}

TEST_CASE("registerSpecial maps every canonical special token type", "[token][special][canonical][usage]")
{
    SpecialTokens special;
    special.registerSpecial("bos", 1, SpecialTokenType::Bos);
    special.registerSpecial("eos", 2, SpecialTokenType::Eos);
    special.registerSpecial("eot", 3, SpecialTokenType::Eot);
    special.registerSpecial("pad", 4, SpecialTokenType::Pad);
    special.registerSpecial("unk", 5, SpecialTokenType::Unk);
    special.registerSpecial("mask", 6, SpecialTokenType::Mask);
    special.registerSpecial("prefix", 7, SpecialTokenType::Prefix);
    special.registerSpecial("suffix", 8, SpecialTokenType::Suffix);
    special.registerSpecial("middle", 9, SpecialTokenType::Middle);
    special.registerSpecial("cls", 10, SpecialTokenType::Cls);
    special.registerSpecial("sep", 11, SpecialTokenType::Sep);

    REQUIRE(special.bosId() == 1);
    REQUIRE(special.eosId() == 2);
    REQUIRE(special.eotId() == 3);
    REQUIRE(special.padId() == 4);
    REQUIRE(special.unkId() == 5);
    REQUIRE(special.maskId() == 6);
    REQUIRE(special.prefixId() == 7);
    REQUIRE(special.suffixId() == 8);
    REQUIRE(special.middleId() == 9);
    REQUIRE(special.clsId() == 10);
    REQUIRE(special.sepId() == 11);
}

TEST_CASE("Named special token without canonical type remains special", "[token][special][named][usage]")
{
    SpecialTokens special;
    special.registerSpecial("custom_special", 77);

    REQUIRE(special.isSpecial(77));
    REQUIRE(special.findByName("custom_special") == 77);
    REQUIRE(special.nameById(77) == "custom_special");
    REQUIRE(special.bosId() == kInvalidToken);
    REQUIRE(special.eosId() == kInvalidToken);
}

TEST_CASE("Reassigning a special name removes the old reverse mapping", "[token][special][invariant][usage]")
{
    SpecialTokens special;
    special.registerSpecial("bos_token", 10, SpecialTokenType::Bos);

    REQUIRE(special.nameById(10) == "bos_token");

    special.registerSpecial("bos_token", 20, SpecialTokenType::Bos);

    REQUIRE(special.findByName("bos_token") == 20);
    REQUIRE(special.nameById(20) == "bos_token");
    REQUIRE(special.nameById(10).empty());
    REQUIRE(special.bosId() == 20);
}

TEST_CASE("Reassigning a special ID removes the old name mapping", "[token][special][invariant][usage]")
{
    SpecialTokens special;
    special.registerSpecial("first_name", 55);

    REQUIRE(special.findByName("first_name") == 55);

    special.registerSpecial("second_name", 55);

    REQUIRE_FALSE(special.findByName("first_name").has_value());
    REQUIRE(special.findByName("second_name") == 55);
    REQUIRE(special.nameById(55) == "second_name");
    REQUIRE(special.isSpecial(55));
}

TEST_CASE("Changing canonical ID removes stale canonical special classification", "[token][special][canonical][invariant]")
{
    SpecialTokens special;
    special.setBosId(100);

    REQUIRE(special.isSpecial(100));

    special.setBosId(200);

    REQUIRE(special.bosId() == 200);
    REQUIRE(special.isSpecial(200));
    REQUIRE_FALSE(special.isSpecial(100));
}

TEST_CASE("Named special token remains special after canonical role moves", "[token][special][canonical][named]")
{
    SpecialTokens special;
    special.registerSpecial("old_bos", 100, SpecialTokenType::Bos);

    REQUIRE(special.isSpecial(100));

    special.setBosId(200);

    REQUIRE(special.bosId() == 200);
    REQUIRE(special.isSpecial(200));

    // Token 100 still has a registered special-token name, so moving
    // the BOS role must not remove it from the special-token set.
    REQUIRE(special.isSpecial(100));
    REQUIRE(special.findByName("old_bos") == 100);
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("SpecialTokens starts completely unassigned", "[token][special][edge]")
{
    SpecialTokens special;

    REQUIRE(special.bosId() == kInvalidToken);
    REQUIRE(special.eosId() == kInvalidToken);
    REQUIRE(special.eotId() == kInvalidToken);
    REQUIRE(special.padId() == kInvalidToken);
    REQUIRE(special.unkId() == kInvalidToken);
    REQUIRE(special.maskId() == kInvalidToken);
    REQUIRE(special.clsId() == kInvalidToken);
    REQUIRE(special.sepId() == kInvalidToken);
    REQUIRE(special.prefixId() == kInvalidToken);
    REQUIRE(special.suffixId() == kInvalidToken);
    REQUIRE(special.middleId() == kInvalidToken);
    REQUIRE_FALSE(special.isSpecial(kInvalidToken));
    REQUIRE_FALSE(special.findByName("missing").has_value());
    REQUIRE(special.nameById(kInvalidToken).empty());
}

TEST_CASE("registerSpecial ignores invalid IDs and empty names", "[token][special][edge]")
{
    SpecialTokens special;
    special.registerSpecial("", 10, SpecialTokenType::Bos);
    special.registerSpecial("invalid", kInvalidToken, SpecialTokenType::Bos);

    REQUIRE_FALSE(special.findByName("").has_value());
    REQUIRE_FALSE(special.findByName("invalid").has_value());
    REQUIRE(special.bosId() == kInvalidToken);
    REQUIRE_FALSE(special.isSpecial(10));
}

TEST_CASE("Setting canonical ID to invalid clears its special classification", "[token][special][edge][canonical]")
{
    SpecialTokens special;
    special.setMaskId(123);

    REQUIRE(special.maskId() == 123);
    REQUIRE(special.isSpecial(123));

    special.setMaskId(kInvalidToken);

    REQUIRE(special.maskId() == kInvalidToken);
    REQUIRE_FALSE(special.isSpecial(123));
}

TEST_CASE("Canonical ID shared by multiple roles remains special until all roles move", "[token][special][edge][canonical]")
{
    SpecialTokens special;
    special.setBosId(25);
    special.setEosId(25);

    REQUIRE(special.isSpecial(25));

    special.setBosId(30);

    REQUIRE(special.bosId() == 30);
    REQUIRE(special.eosId() == 25);

    // EOS still owns token 25 canonically.
    REQUIRE(special.isSpecial(25));

    special.setEosId(31);

    REQUIRE_FALSE(special.isSpecial(25));
    REQUIRE(special.isSpecial(30));
    REQUIRE(special.isSpecial(31));
}

TEST_CASE("reset clears canonical IDs names reverse mappings and special set", "[token][special][reset][edge]")
{
    SpecialTokens special;
    special.registerSpecial("bos_token", 1, SpecialTokenType::Bos);
    special.registerSpecial("custom", 2);
    special.setMaskId(3);

    REQUIRE(special.isSpecial(1));
    REQUIRE(special.isSpecial(2));
    REQUIRE(special.isSpecial(3));

    special.reset();

    REQUIRE(special.bosId() == kInvalidToken);
    REQUIRE(special.eosId() == kInvalidToken);
    REQUIRE(special.eotId() == kInvalidToken);
    REQUIRE(special.padId() == kInvalidToken);
    REQUIRE(special.unkId() == kInvalidToken);
    REQUIRE(special.maskId() == kInvalidToken);
    REQUIRE(special.clsId() == kInvalidToken);
    REQUIRE(special.sepId() == kInvalidToken);
    REQUIRE(special.prefixId() == kInvalidToken);
    REQUIRE(special.suffixId() == kInvalidToken);
    REQUIRE(special.middleId() == kInvalidToken);
    REQUIRE_FALSE(special.findByName("bos_token").has_value());
    REQUIRE_FALSE(special.findByName("custom").has_value());
    REQUIRE(special.nameById(1).empty());
    REQUIRE(special.nameById(2).empty());
    REQUIRE_FALSE(special.isSpecial(1));
    REQUIRE_FALSE(special.isSpecial(2));
    REQUIRE_FALSE(special.isSpecial(3));
}