#pragma once

#include <string_view>

namespace bsfchat {

// Matrix Client-Server API paths
namespace api_path {
    constexpr std::string_view kVersions = "/_matrix/client/versions";
    constexpr std::string_view kLogin = "/_matrix/client/v3/login";
    constexpr std::string_view kRegister = "/_matrix/client/v3/register";
    constexpr std::string_view kLogout = "/_matrix/client/v3/logout";
    constexpr std::string_view kLogoutAll = "/_matrix/client/v3/logout/all";
    // Authenticated password change. Requires re-authentication with the
    // current password, and revokes the account's other sessions by default.
    constexpr std::string_view kPasswordChange = "/_matrix/client/v3/account/password";
    // Exchanges a refresh token for a fresh access/refresh pair.
    constexpr std::string_view kRefresh = "/_matrix/client/v3/refresh";
    constexpr std::string_view kWhoami = "/_matrix/client/v3/account/whoami";
    constexpr std::string_view kSync = "/_matrix/client/v3/sync";
    constexpr std::string_view kJoinedRooms = "/_matrix/client/v3/joined_rooms";
    constexpr std::string_view kCreateRoom = "/_matrix/client/v3/createRoom";
    constexpr std::string_view kMediaUpload = "/_matrix/media/v3/upload";
    // Moderation audit log (read-only, server-scope admin permission).
    // bsfchat.* namespaced because it has no Matrix-spec equivalent.
    constexpr std::string_view kAuditLog = "/_matrix/client/v3/bsfchat/audit_log";
    // The server-wide ban list (read-only; bans are placed and lifted through
    // POST /rooms/{id}/ban and /unban, which is where the rank check lives).
    constexpr std::string_view kServerBans = "/_matrix/client/v3/bsfchat/server_bans";
    // Bot accounts. bsfchat.* namespaced for the same reason the two above are:
    // Matrix has no concept of a bot account, so these are ours and must not
    // squat a path the spec might later define. A bot IS an ordinary user
    // account, so there is no bot-flavoured variant of any other endpoint —
    // everything else a bot does goes through the normal routes and the normal
    // bearer-token middleware.
    constexpr std::string_view kBots = "/_matrix/client/v3/bsfchat/bots";
    // Server role definitions: list, create, edit, delete, reposition.
    //
    // bsfchat.* namespaced for the same reason the three above are — Matrix has
    // no role model, only m.room.power_levels, which this server does not use as
    // its authority.
    //
    // These endpoints do not introduce a second home for role data. The
    // authoritative copy is still the `bsfchat.server.roles` server-scoped state
    // event, and every write here is a read-modify-write of that one document
    // performed BY THE SERVER, checked by the same
    // PermissionsEngine::may_edit_role_definitions that guards the wholesale
    // PUT. What they remove is the requirement that the CALLER assemble the
    // whole document, which is what made roles unusable from a bot: a delegated
    // MANAGE_ROLES holder cannot faithfully echo back role definitions it is not
    // allowed to touch, and two editors racing on a wholesale PUT silently lose
    // one of the two edits.
    constexpr std::string_view kRoles = "/_matrix/client/v3/bsfchat/roles";
    // Roles the CALLER may add to or remove from themselves. PUT/DELETE
    // /self_roles/{roleId}. Separate path rather than a verb on kRoles because
    // the authority is completely different: kRoles is MANAGE_ROLES plus rank,
    // this is "the role says anyone may take it" plus a permission-containment
    // rule, and conflating the two in one handler is how the containment rule
    // would eventually get skipped on one branch.
    constexpr std::string_view kSelfRoles = "/_matrix/client/v3/bsfchat/self_roles";

    // Parameterized paths (use fmt or string concat with room/event IDs)
    constexpr std::string_view kRoomPrefix = "/_matrix/client/v3/rooms/";
    constexpr std::string_view kMediaDownload = "/_matrix/media/v3/download/";
    constexpr std::string_view kMediaThumbnail = "/_matrix/media/v3/thumbnail/";
    constexpr std::string_view kProfile = "/_matrix/client/v3/profile/";
    constexpr std::string_view kJoinByAlias = "/_matrix/client/v3/join/";
    constexpr std::string_view kTyping = "/_matrix/client/v3/rooms/"; // + roomId + /typing/ + userId
    constexpr std::string_view kPresence = "/_matrix/client/v3/presence/"; // + userId + /status
} // namespace api_path

// Matrix event types
namespace event_type {
    constexpr std::string_view kRoomCreate = "m.room.create";
    constexpr std::string_view kRoomName = "m.room.name";
    constexpr std::string_view kRoomTopic = "m.room.topic";
    constexpr std::string_view kRoomAvatar = "m.room.avatar";
    constexpr std::string_view kRoomMember = "m.room.member";
    constexpr std::string_view kRoomMessage = "m.room.message";
    // Account data, not a room event: the user's peer -> DM room ids map.
    constexpr std::string_view kDirect = "m.direct";
    constexpr std::string_view kRoomJoinRules = "m.room.join_rules";
    constexpr std::string_view kRoomPowerLevels = "m.room.power_levels";
    constexpr std::string_view kRoomCanonicalAlias = "m.room.canonical_alias";
    constexpr std::string_view kRoomHistoryVisibility = "m.room.history_visibility";
    constexpr std::string_view kRoomVoice = "m.room.voice";
    constexpr std::string_view kCallInvite = "m.call.invite";
    constexpr std::string_view kCallAnswer = "m.call.answer";
    constexpr std::string_view kCallCandidates = "m.call.candidates";
    constexpr std::string_view kCallHangup = "m.call.hangup";
    // Mid-call SDP renegotiation (BSFChat extension, not Matrix spec —
    // hence the bsfchat.* namespace). Carries {call_id, description:
    // {type: "offer"|"answer", sdp}, version}. Used to add RTP video
    // m-lines to an established call; only sent to peers that
    // advertised `bsfchat_caps.video_rtp` in their invite/answer, so
    // legacy clients never see it.
    constexpr std::string_view kCallNegotiate = "bsfchat.call.negotiate";
    constexpr std::string_view kCallMember = "m.call.member";
    constexpr std::string_view kTyping = "m.typing";
    constexpr std::string_view kPresence = "m.presence";
    constexpr std::string_view kRoomCategory = "bsfchat.room.category";
    constexpr std::string_view kRoomType = "bsfchat.room.type";
    constexpr std::string_view kServerInfo = "bsfchat.server.info";
    constexpr std::string_view kServerRoles = "bsfchat.server.roles";
    constexpr std::string_view kMemberRoles = "bsfchat.member.roles";
    constexpr std::string_view kChannelSettings = "bsfchat.channel.settings";
    constexpr std::string_view kChannelPermissions = "bsfchat.channel.permissions";
    constexpr std::string_view kRoomRedaction = "m.room.redaction";
    // Emoji reaction. Content is
    //   {"m.relates_to": {"rel_type": "m.annotation", "event_id": "$...", "key": "👍"}}
    // which is what the desktop client sends and what its MessageModel folds
    // into the target message. Named here because the server now has to decide
    // per event type what may be sent to a room, and matching a bare string
    // literal in that table is how a type quietly ends up ungated.
    constexpr std::string_view kReaction = "m.reaction";
    constexpr std::string_view kRelAnnotation = "m.annotation";
    constexpr std::string_view kRoomPinnedEvents = "m.room.pinned_events";
    // Server-wide screen-share policy (max quality preset). Written by
    // admins via setMaxScreenShareQuality; read by every client on sync
    // to clamp users' locally-chosen quality downward.
    constexpr std::string_view kServerScreenShare = "bsfchat.server.screenshare";
} // namespace event_type

// Message types (m.room.message msgtype field)
namespace msg_type {
    constexpr std::string_view kText = "m.text";
    constexpr std::string_view kEmote = "m.emote";
    constexpr std::string_view kNotice = "m.notice";
    constexpr std::string_view kImage = "m.image";
    constexpr std::string_view kFile = "m.file";
    constexpr std::string_view kAudio = "m.audio";
    constexpr std::string_view kVideo = "m.video";
} // namespace msg_type

// Membership states
namespace membership {
    constexpr std::string_view kJoin = "join";
    constexpr std::string_view kLeave = "leave";
    constexpr std::string_view kInvite = "invite";
    constexpr std::string_view kBan = "ban";
    constexpr std::string_view kKnock = "knock";
} // namespace membership

// Join rules
namespace join_rule {
    constexpr std::string_view kPublic = "public";
    constexpr std::string_view kInvite = "invite";
    constexpr std::string_view kKnock = "knock";
} // namespace join_rule

// Supported Matrix spec versions
namespace spec {
    constexpr std::string_view kVersion = "v1.12";
} // namespace spec

// Default limits
// Bot accounts.
//
// A bot is a USER — same users row, same roles, same permission evaluation, same
// bearer-token middleware — distinguished only by `users.kind` and by living in a
// reserved localpart namespace. Nothing here describes a parallel account type;
// it describes the two facts a client or a handler needs in order to tell one
// apart from a person.
namespace bot {

// Every bot localpart starts with this, and no human account may. The namespace
// is the point, not the cosmetics: it means "is this a bot" has an answer that
// does not depend on a database round trip, and — because a registration handler
// refuses the prefix — that a person can never be handed a user id that a client
// will badge as a bot. Exactly the same reasoning as the existing "server" and
// "oidc_" reservations in AuthHandler::handle_register, and the reservation is
// enforced in that same place so there is one list, not two that can drift.
constexpr std::string_view kLocalpartPrefix = "bot_";

// The key /profile/{userId} and /account/whoami carry for a bot.
//
// Surfaced as a profile field rather than as new membership state on purpose: a
// client already fetches a profile to render a name and an avatar, whereas a new
// m.room.member field would have to be backfilled into every room a bot is in
// and would still be absent in rooms it has not joined. Bot-ness is a property of
// the ACCOUNT, so it belongs on the account's profile.
//
// Emitted only when true. Absent means "not a bot", the same way an unset
// displayname or nickname is simply absent rather than "".
constexpr std::string_view kProfileKey = "bsfchat.bot";

// Is this user id a bot's, judged from the id alone?
//
// Authoritative BECAUSE of the reservation, not in spite of it: registration
// refuses the prefix and bot creation requires it, so the two sets are disjoint
// by construction and no account can sit on the wrong side of this answer.
//
// Provided so a caller on a hot path — a rate limiter deciding which bucket a
// request belongs in, a renderer deciding whether to draw a badge — can classify
// a caller without a database round trip per request. Where the answer must
// survive somebody changing that rule, ask the store (SqliteStore::is_bot),
// which reads users.kind and is the fact rather than the convention.
//
// Takes a full user id ("@bot_deploy:example.com"), not a localpart.
constexpr bool is_bot_user_id(std::string_view user_id) {
    if (user_id.size() < 1 + kLocalpartPrefix.size()) return false;
    if (user_id.front() != '@') return false;
    return user_id.substr(1, kLocalpartPrefix.size()) == kLocalpartPrefix;
}

} // namespace bot

namespace limits {
    constexpr int kDefaultSyncTimeoutMs = 30000;
    constexpr int kMaxSyncTimeoutMs = 300000;
    constexpr int kDefaultTimelineLimit = 20;
    constexpr int kDefaultMessagesLimit = 50;
    constexpr int kMaxMessagesLimit = 1000;
    constexpr size_t kMaxUploadSizeMb = 50;
    constexpr size_t kMaxUsernameLength = 64;
    constexpr size_t kMinPasswordLength = 8;
    // Ceiling on an m.reaction's `key` — the emoji a client groups and counts
    // reactions by. Generous: a ZWJ sequence with skin-tone modifiers runs to
    // tens of bytes. A storage bound, not an emoji validator; what counts as an
    // emoji is a client concern and a moving target. Unbounded, it was an
    // attacker-chosen string stored per reaction and pushed to every member of
    // the room on every sync.
    constexpr size_t kMaxReactionKeyLength = 64;
    // Ceiling on `m.mentions.user_ids` entries in a single event. A mention is
    // a write per target plus a push-queue row per target's pusher, so an
    // unbounded list is a cheap amplification primitive. Well above any real
    // message; a client hitting this is broken or hostile.
    constexpr size_t kMaxMentionsPerEvent = 50;
    // Ceilings for POST /search.
    constexpr int kDefaultSearchLimit = 20;
    constexpr int kMaxSearchLimit = 100;
    constexpr size_t kMaxSearchTermLength = 512;
    // Page sizes for the moderation audit log.
    constexpr int kDefaultAuditLimit = 50;
    constexpr int kMaxAuditLimit = 200;
    // Page sizes for the server-wide ban list. Smaller ceiling than the audit
    // log: a ban list is a working set an operator scrolls, not a history they
    // grep, and the client renders every row.
    constexpr int kDefaultServerBanLimit = 100;
    constexpr int kMaxServerBanLimit = 500;
    // A bot's operator-supplied description ("what is this thing for"). Bounded
    // because it is free text that every listing renders; the value is generous
    // enough for a sentence and a link, which is all it is for.
    constexpr size_t kMaxBotDescriptionLength = 512;
    // Role definitions. The whole role list is ONE state event that every
    // client holds in memory and every permission evaluation walks, so these
    // are correctness bounds and not just tidiness: without them a caller with
    // MANAGE_ROLES can grow that event without limit and make every request on
    // the server slower for everyone.
    constexpr size_t kMaxRoles = 250;
    constexpr size_t kMaxRoleNameLength = 100;
    // Positions are a ladder, not an index — they are sparse by design
    // (0/10/100 at bootstrap) and nothing requires them to be unique or
    // contiguous. The ceiling exists so arithmetic on them cannot be pushed
    // anywhere near overflow, not because the range is meaningful.
    constexpr int kMaxRolePosition = 1000000;
} // namespace limits

} // namespace bsfchat
