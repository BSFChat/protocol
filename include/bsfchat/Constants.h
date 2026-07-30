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
namespace limits {
    constexpr int kDefaultSyncTimeoutMs = 30000;
    constexpr int kMaxSyncTimeoutMs = 300000;
    constexpr int kDefaultTimelineLimit = 20;
    constexpr int kDefaultMessagesLimit = 50;
    constexpr int kMaxMessagesLimit = 1000;
    constexpr size_t kMaxUploadSizeMb = 50;
    constexpr size_t kMaxUsernameLength = 64;
    constexpr size_t kMinPasswordLength = 8;
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
} // namespace limits

} // namespace bsfchat
