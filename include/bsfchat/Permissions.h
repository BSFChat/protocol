#pragma once

#include <bsfchat/Constants.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bsfchat {

// Discord-style permission bitfield. Each flag gates a capability either
// server-wide (via role) or per-channel (via override). Stored on the wire as
// a hex string to survive JSON's 53-bit number limit.
namespace permission {

using Flags = std::uint64_t;

// Viewing & reading
constexpr Flags kViewChannel      = 1ULL <<  0;

// Messaging
constexpr Flags kSendMessages     = 1ULL <<  1;
constexpr Flags kAttachFiles      = 1ULL <<  2;
constexpr Flags kEmbedLinks       = 1ULL <<  3;
constexpr Flags kManageMessages   = 1ULL <<  4;  // delete others', bypass slowmode

// Channel & server management
constexpr Flags kManageChannels   = 1ULL <<  5;
constexpr Flags kManageRoles      = 1ULL <<  6;
constexpr Flags kManageServer     = 1ULL << 10;

// Membership moderation
constexpr Flags kKickMembers      = 1ULL <<  7;
constexpr Flags kBanMembers       = 1ULL <<  8;

// Mentions
constexpr Flags kMentionEveryone  = 1ULL <<  9;

// Nicknames
constexpr Flags kChangeNickname   = 1ULL << 11;
constexpr Flags kManageNicknames  = 1ULL << 12;

// Bot accounts: create one, list them, rotate its token, deactivate it.
//
// Its own flag rather than a corner of MANAGE_SERVER, because what it hands out
// is qualitatively different from the rest of that flag's surface. A bot token
// is a non-expiring bearer credential for a full user account — there is no
// human behind it to re-authenticate, so it does not ride the access-token
// expiry slide — and whoever can mint one can mint an identity that then picks
// up whatever roles it is given. That is a capability an owner should be able to
// delegate (or withhold) on its own, separately from "can edit the server name".
//
// Deliberately NOT in kEveryoneDefault: the default role must never be able to
// manufacture accounts. It IS in kAllFlags, so ADMINISTRATOR short-circuits to
// it like every other flag — which is also why an already-seeded Admin role on
// an existing deployment picks this up without a re-seed.
constexpr Flags kManageBots       = 1ULL << 13;

// Reactions
//
// Bit 14. Bit 13 is MANAGE_BOTS on the bot-accounts branch — do not reuse it.
//
// Separate from SEND_MESSAGES because muting somebody was not actually
// possible before this: the server applied no permission check to m.reaction
// at all, so a member with SEND_MESSAGES denied could still react. Gating
// reactions on SEND_MESSAGES would have closed that hole while quietly
// conflating two things moderators treat differently — a read-mostly channel
// where everyone may react but only a few may post is an ordinary
// arrangement, and so is muting someone completely.
//
// IN kEveryoneDefault, deliberately. The point of this flag is that denying
// reactions becomes possible, not that reacting becomes a privilege, so a
// server that upgrades must see no change in what anyone can do. Note that
// kEveryoneDefault only seeds a NEW deployment: an existing one has its
// @everyone permissions stored as a number in its server.roles event, and the
// server backfills this bit into every role that already had SEND_MESSAGES
// (see bootstrap_roles) precisely so the upgrade is invisible.
constexpr Flags kAddReactions     = 1ULL << 14;

// God mode
constexpr Flags kAdministrator    = 1ULL << 15;

// Default capabilities for the built-in @everyone role.
constexpr Flags kEveryoneDefault =
    kViewChannel | kSendMessages | kAttachFiles | kEmbedLinks | kChangeNickname |
    kAddReactions;

// Mask covering every flag we currently define — used for the ADMINISTRATOR
// short-circuit and for UI "check all" toggles. Expand as new flags are added.
constexpr Flags kAllFlags =
    kViewChannel | kSendMessages | kAttachFiles | kEmbedLinks |
    kManageMessages | kManageChannels | kManageRoles | kKickMembers |
    kBanMembers | kMentionEveryone | kManageServer | kChangeNickname |
    kManageNicknames | kManageBots | kAddReactions | kAdministrator;

inline bool has(Flags flags, Flags p) {
    return (flags & p) == p;
}

// Does the implicit @everyone role apply to this account?
//
// @everyone is the default role FOR PEOPLE WHO JOIN THE SERVER. A permission
// evaluation applies it to an account whether or not the account's assignment
// names it, which is what makes it a default rather than a role — and for a bot
// that default is the wrong one. A bot is not somebody who joined; it is an
// account an administrator manufactured for one job, and it is already excluded
// from the other two things joining confers: it cannot log in with a password,
// and it is excluded from channel auto-join. The implicit default role is the
// third exclusion, and it is the one with consequences.
//
// Concretely: kEveryoneDefault is VIEW_CHANNEL | SEND_MESSAGES on a typical
// server, so a bot minted to answer questions in one channel could read and post
// in EVERY channel on the server the moment it let itself into them — and no
// role assignment could narrow that, because a role only ever ADDS bits. There
// is no assignment, on any account, that subtracts something @everyone already
// grants. So "this bot may see these three channels and nothing else" was not a
// sentence the permission model could say at all, however it was configured.
//
// Withholding the implicit grant makes @everyone ORDINARY for a bot: still a
// role, still assignable by id like any other. "Let this bot do whatever a
// member can" stays available — it just has to be said on purpose, rather than
// being what an operator gets for saying nothing. A per-channel override keyed
// `user:<bot id>` then grants the channels it is actually for, which is the same
// mechanism that makes a channel private for a person.
//
// Note what this does NOT change: the @everyone channel OVERRIDE still applies
// to a bot, because an override is a statement about a CHANNEL ("this one is
// open to everybody") rather than about who holds which role — the same reason
// compute() applies it unconditionally today.
//
// KEYED ON THE USER ID, not on a database lookup, and safe to be: the `bot_`
// namespace is closed on every path that can create an account — registration
// refuses the prefix outright, the OIDC auto-create path mints `oidc_`
// localparts, and bot creation refuses a localpart outside it. So a real bot
// always classifies as one, which is the direction that matters here. The
// failure this must not have is a bot quietly keeping the default; a human who
// somehow held a `bot_` id would instead lose it, which is visible immediately
// and grants nobody anything.
constexpr bool inherits_everyone_role(std::string_view user_id) {
    return !bot::is_bot_user_id(user_id);
}

// Well-known role IDs seeded at server bootstrap.
namespace role_id {
constexpr const char* kEveryone = "everyone";
constexpr const char* kModerator = "mod";
constexpr const char* kAdmin = "admin";
} // namespace role_id

// Convert a Flags value to a "0x..." hex string (lowercase, no padding beyond
// what's needed). Used for all wire serialization of permission fields.
std::string flags_to_hex(Flags flags);

// Parse a hex string like "0x01ff" or "1ff" into Flags. Returns 0 on failure.
Flags flags_from_hex(const std::string& s);

} // namespace permission

} // namespace bsfchat
