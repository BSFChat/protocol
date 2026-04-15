#pragma once

#include <string_view>

namespace bsfchat {

// Matrix Client-Server API paths
namespace api_path {
    constexpr std::string_view kVersions = "/_matrix/client/versions";
    constexpr std::string_view kLogin = "/_matrix/client/v3/login";
    constexpr std::string_view kRegister = "/_matrix/client/v3/register";
    constexpr std::string_view kLogout = "/_matrix/client/v3/logout";
    constexpr std::string_view kSync = "/_matrix/client/v3/sync";
    constexpr std::string_view kJoinedRooms = "/_matrix/client/v3/joined_rooms";
    constexpr std::string_view kCreateRoom = "/_matrix/client/v3/createRoom";
    constexpr std::string_view kMediaUpload = "/_matrix/media/v3/upload";

    // Parameterized paths (use fmt or string concat with room/event IDs)
    constexpr std::string_view kRoomPrefix = "/_matrix/client/v3/rooms/";
    constexpr std::string_view kMediaDownload = "/_matrix/media/v3/download/";
    constexpr std::string_view kMediaThumbnail = "/_matrix/media/v3/thumbnail/";
    constexpr std::string_view kProfile = "/_matrix/client/v3/profile/";
    constexpr std::string_view kJoinByAlias = "/_matrix/client/v3/join/";
    constexpr std::string_view kTyping = "/_matrix/client/v3/rooms/"; // + roomId + /typing/ + userId
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
    constexpr std::string_view kCallMember = "m.call.member";
    constexpr std::string_view kTyping = "m.typing";
    constexpr std::string_view kRoomCategory = "bsfchat.room.category";
    constexpr std::string_view kRoomType = "bsfchat.room.type";
    constexpr std::string_view kServerRoles = "bsfchat.server.roles";
    constexpr std::string_view kMemberRoles = "bsfchat.member.roles";
    constexpr std::string_view kChannelSettings = "bsfchat.channel.settings";
    constexpr std::string_view kChannelPermissions = "bsfchat.channel.permissions";
    constexpr std::string_view kRoomRedaction = "m.room.redaction";
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
} // namespace limits

} // namespace bsfchat
