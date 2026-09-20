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
