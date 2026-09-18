#pragma once

#include <cstdint>
#include <string>
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
    kManageNicknames | kAddReactions | kAdministrator;

inline bool has(Flags flags, Flags p) {
    return (flags & p) == p;
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
