#include <gtest/gtest.h>
#include "bsfchat/Identifiers.h"

#include <string>
#include <unordered_set>

using namespace bsfchat;

TEST(UserId, ParseValid) {
    auto id = UserId::parse("@alice:example.com");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->localpart, "alice");
    EXPECT_EQ(id->server_name, "example.com");
    EXPECT_EQ(id->to_string(), "@alice:example.com");
}

TEST(UserId, ParseWithPort) {
    auto id = UserId::parse("@bob:localhost:8448");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->localpart, "bob");
    EXPECT_EQ(id->server_name, "localhost:8448");
}

TEST(UserId, ParseInvalid) {
    EXPECT_FALSE(UserId::parse("alice:example.com"));  // no sigil
    EXPECT_FALSE(UserId::parse("@:example.com"));       // empty localpart
    EXPECT_FALSE(UserId::parse("@alice:"));             // empty server
    EXPECT_FALSE(UserId::parse("@alice"));              // no colon
    EXPECT_FALSE(UserId::parse(""));                    // empty
    EXPECT_FALSE(UserId::parse("@Alice:example.com"));  // uppercase
}

TEST(UserId, IsValid) {
    EXPECT_TRUE(UserId::is_valid("@alice:example.com"));
    EXPECT_FALSE(UserId::is_valid("not_a_user_id"));
}

TEST(RoomId, ParseValid) {
    auto id = RoomId::parse("!abc123:example.com");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->localpart, "abc123");
    EXPECT_EQ(id->server_name, "example.com");
    EXPECT_EQ(id->to_string(), "!abc123:example.com");
}

TEST(RoomId, ParseInvalid) {
    EXPECT_FALSE(RoomId::parse("abc123:example.com"));
    EXPECT_FALSE(RoomId::parse("!:example.com"));
    EXPECT_FALSE(RoomId::parse(""));
}

TEST(RoomAlias, ParseValid) {
    auto alias = RoomAlias::parse("#general:example.com");
    ASSERT_TRUE(alias.has_value());
    EXPECT_EQ(alias->localpart, "general");
    EXPECT_EQ(alias->server_name, "example.com");
    EXPECT_EQ(alias->to_string(), "#general:example.com");
}

TEST(EventId, ParseValid) {
    auto id = EventId::parse("$abc123xyz:example.com");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->to_string(), "$abc123xyz:example.com");
}

TEST(EventId, ParseInvalid) {
    EXPECT_FALSE(EventId::parse("$"));   // too short
    EXPECT_FALSE(EventId::parse("abc")); // no sigil
    EXPECT_FALSE(EventId::parse(""));
}

TEST(Generators, EventIdFormat) {
    auto id = generate_event_id("example.com");
    EXPECT_EQ(id[0], '$');
    EXPECT_NE(id.find(":example.com"), std::string::npos);
}

TEST(Generators, RoomIdFormat) {
    auto id = generate_room_id("example.com");
    EXPECT_EQ(id[0], '!');
    EXPECT_NE(id.find(":example.com"), std::string::npos);
}

TEST(Generators, AccessTokenLength) {
    auto token = generate_access_token();
    EXPECT_GE(token.size(), 40u);
}

TEST(Generators, UniqueTokens) {
    auto a = generate_access_token();
    auto b = generate_access_token();
    EXPECT_NE(a, b);
}

TEST(Generators, DeviceIdFormat) {
    auto id = generate_device_id();
    EXPECT_TRUE(id.starts_with("DEVICE_"));
}

// --- token entropy -----------------------------------------------------------
//
// The generators used to be mt19937 seeded from one 32-bit value, so every
// token was one of 2^32 strings no matter how long it printed. The tests above
// could not see that: two consecutive tokens still differ, and the length was
// always right. What a 32-bit keyspace cannot survive is VOLUME — the birthday
// bound turns it into repeats long before the strings themselves look suspect.

TEST(Generators, TokensDoNotCollideAtVolume) {
    // 500k draws from a 2^32 keyspace collide ~29 times on average, so the old
    // generator failed this essentially always (probability of slipping
    // through ~1e-13). A real 256-bit token space cannot collide here at all,
    // which is why this is an equality against zero and not a threshold.
    constexpr int kDraws = 500'000;
    std::unordered_set<std::string> seen;
    seen.reserve(kDraws * 2);
    int collisions = 0;
    for (int i = 0; i < kDraws; ++i) {
        if (!seen.insert(generate_access_token()).second) ++collisions;
    }
    EXPECT_EQ(collisions, 0);
}

TEST(Generators, AccessTokenIsFullLengthAndInAlphabet) {
    // Length is load-bearing: it is the only thing standing between the token
    // and a brute-force, so a truncation regression must fail here.
    const auto token = generate_access_token();
    EXPECT_EQ(token.size(), 43u); // 43 * 6 bits = 258 bits
    for (char c : token) {
        EXPECT_TRUE((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_')
            << "unexpected character '" << c << "' in access token";
    }
}

TEST(Generators, EveryAlphabetCharacterIsReachable) {
    // Guards the `& 0x3F` fold: an off-by-one in the mask would quietly shrink
    // the alphabet (and so the keyspace) while every other test still passed.
    std::unordered_set<char> seen;
    for (int i = 0; i < 2000; ++i) {
        for (char c : generate_access_token()) seen.insert(c);
    }
    EXPECT_EQ(seen.size(), 64u);
}
