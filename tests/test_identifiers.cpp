#include <gtest/gtest.h>
#include "bsfchat/Identifiers.h"

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
