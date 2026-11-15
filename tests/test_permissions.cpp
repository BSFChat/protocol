// The permission bitfield's own invariants, and the one rule in this header
// that is not a constant: who inherits the implicit @everyone role.
//
// Derived from the constants rather than restating them, except where the
// point IS the literal — kManageBots' bit number is pinned because the desktop
// client carries its own copy of the value and the two must not drift.

#include <gtest/gtest.h>

#include <bsfchat/Constants.h>
#include <bsfchat/Permissions.h>

using namespace bsfchat;

TEST(Permissions, EveryoneDefaultIsContainedInAllFlags) {
    EXPECT_EQ(permission::kEveryoneDefault & ~permission::kAllFlags, 0u);
}

TEST(Permissions, TheDefaultRoleCannotManufactureAccountsOrOwnTheServer) {
    // Both are deliberate absences from kEveryoneDefault, and both are the
    // kind of bit that gets folded in by accident when the mask is edited.
    EXPECT_FALSE(permission::has(permission::kEveryoneDefault, permission::kManageBots));
    EXPECT_FALSE(permission::has(permission::kEveryoneDefault, permission::kAdministrator));
    EXPECT_FALSE(permission::has(permission::kEveryoneDefault, permission::kManageRoles));
}

TEST(Permissions, ABotDoesNotInheritTheImplicitEveryoneRole) {
    // The whole of bot scoping rests on this: a bot's base permissions are the
    // roles it was GIVEN, and nothing else. If this ever returns true for a bot
    // id, every scoped bot silently regains VIEW_CHANNEL and SEND_MESSAGES on
    // every channel on the server.
    EXPECT_FALSE(permission::inherits_everyone_role("@bot_deploy:example.com"));
    EXPECT_FALSE(permission::inherits_everyone_role("@bot_:example.com"));
}

TEST(Permissions, EverybodyElseDoesInheritIt) {
    // The far more damaging direction of the same rule. A human who stopped
    // inheriting @everyone would lose every default permission on the server.
    EXPECT_TRUE(permission::inherits_everyone_role("@alice:example.com"));
    EXPECT_TRUE(permission::inherits_everyone_role("@oidc_abc123:example.com"));
    // "robot" is not "bot_": the prefix is matched, not searched for.
    EXPECT_TRUE(permission::inherits_everyone_role("@robot:example.com"));
    EXPECT_TRUE(permission::inherits_everyone_role("@notabot_x:example.com"));
    // The synthetic server actor, which PermissionsEngine short-circuits anyway.
    EXPECT_TRUE(permission::inherits_everyone_role("@server:example.com"));
    // Nonsense in, no accidental bot out.
    EXPECT_TRUE(permission::inherits_everyone_role(""));
    EXPECT_TRUE(permission::inherits_everyone_role("bot_deploy:example.com")); // no leading @
}

TEST(Permissions, TheRuleIsExactlyTheBotClassifier) {
    // Two expressions of one fact; a divergence would mean a client badging an
    // account as a bot while the server granted it member permissions.
    for (const char* id : {"@bot_a:x", "@alice:x", "@robot:x", "@bot_:x", "", "@"}) {
        EXPECT_EQ(permission::inherits_everyone_role(id), !bot::is_bot_user_id(id))
            << "for id \"" << id << "\"";
    }
}

// flags_from_hex is documented as "returns 0 on failure". It used strtoull,
// which reads "-1" (and any overflowing value) as all ones — ADMINISTRATOR
// included — and stops quietly at trailing junk.
TEST(Permissions, FlagsFromHexIsStrict) {
    using bsfchat::permission::flags_from_hex;
    using bsfchat::permission::flags_to_hex;
    EXPECT_EQ(flags_from_hex("-1"), 0u);
    EXPECT_EQ(flags_from_hex("0x-1"), 0u);
    EXPECT_EQ(flags_from_hex("+1ff"), 0u);
    EXPECT_EQ(flags_from_hex(" 1ff"), 0u);
    EXPECT_EQ(flags_from_hex("1ffzz"), 0u);
    EXPECT_EQ(flags_from_hex("0x"), 0u);
    EXPECT_EQ(flags_from_hex(""), 0u);
    EXPECT_EQ(flags_from_hex("1" + std::string(16, '0')), 0u); // 17 digits: overflow
    // Valid forms still parse, and round-trip with flags_to_hex.
    EXPECT_EQ(flags_from_hex("0x01ff"), 0x1ffu);
    EXPECT_EQ(flags_from_hex("1FF"), 0x1ffu);
    EXPECT_EQ(flags_from_hex("ffffffffffffffff"), ~bsfchat::permission::Flags{0});
    for (bsfchat::permission::Flags f : {bsfchat::permission::Flags{0},
                                         bsfchat::permission::kEveryoneDefault,
                                         bsfchat::permission::kAllFlags}) {
        EXPECT_EQ(flags_from_hex(flags_to_hex(f)), f);
    }
}

// flags_from_hex's 0-on-failure is indistinguishable from a real "0x0". The
// optional variant is the one that can tell them apart, and flags_from_hex is
// exactly it with the failure folded to 0.
TEST(Permissions, TryFlagsFromHexDistinguishesFailureFromZero) {
    using bsfchat::permission::try_flags_from_hex;
    using bsfchat::permission::flags_from_hex;
    ASSERT_TRUE(try_flags_from_hex("0x0").has_value());
    EXPECT_EQ(*try_flags_from_hex("0x0"), 0u);
    EXPECT_EQ(*try_flags_from_hex("0"), 0u);
    EXPECT_EQ(*try_flags_from_hex("0x1ff"), 0x1ffu);
    for (const char* bad : {"", "0x", "-1", "0x-1", "+1", " 1", "1 ", "zz", "0x1g",
                            "11111111111111111"}) {
        EXPECT_FALSE(try_flags_from_hex(bad).has_value()) << "for \"" << bad << "\"";
        EXPECT_EQ(flags_from_hex(bad), 0u) << "for \"" << bad << "\"";
    }
    // As written: undefined bits are NOT masked here. Masking is the caller's
    // decision (the event parsers in MatrixTypes make it).
    EXPECT_EQ(*try_flags_from_hex("ffffffffffffffff"), ~bsfchat::permission::Flags{0});
}
