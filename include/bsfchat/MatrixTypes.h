#pragma once

#include <nlohmann/json.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace bsfchat {

// Login request/response
struct LoginRequest {
    std::string type; // "m.login.password" or "m.login.token"
    struct {
        std::string type; // "m.id.user"
        std::string user;
    } identifier;
    std::string password;
    std::string token;                         // for OIDC token login
    std::optional<std::string> device_id;
    std::optional<std::string> initial_device_display_name;
    // Client opts in to refresh tokens. When false the server issues a
    // long-lived access token only (see kDefaultAccessTokenLifetimeMs), which
    // is what pre-refresh clients rely on.
    bool refresh_token = false;
};

struct LoginResponse {
    std::string user_id;
    std::string access_token;
    std::string device_id;
    // Only present when the client asked for a refresh token.
    std::optional<std::string> refresh_token;
    // Remaining validity of `access_token`. Advisory: the server slides the
    // expiry forward while the session stays in use.
    std::optional<int64_t> expires_in_ms;
};

// Registration
struct RegisterRequest {
    std::string username;
    std::string password;
    std::optional<std::string> device_id;
    std::optional<std::string> initial_device_display_name;
    bool refresh_token = false;
};

// Room creation
struct CreateRoomRequest {
    std::optional<std::string> name;
    std::optional<std::string> topic;
    std::optional<std::string> room_alias_name;
    std::string visibility = "private"; // "public" or "private"
    std::vector<std::string> invite;
    std::optional<std::string> preset; // "private_chat", "public_chat", "trusted_private_chat"
    std::optional<bool> is_category;
    std::optional<std::string> parent_id;
    std::optional<int> sort_order;
    std::optional<bool> is_direct; // Matrix DM flag — stored on the room
};

struct CreateRoomResponse {
    std::string room_id;
};

// Events
struct EventContent {
    nlohmann::json data; // flexible content — varies by event type
};

struct RoomEvent {
    std::string event_id;
    std::string room_id;
    std::string sender;
    std::string type;
    EventContent content;
    int64_t origin_server_ts = 0;
    std::optional<std::string> state_key; // present for state events
    std::optional<EventContent> unsigned_data;
};

// Message content (for m.room.message events)
struct MessageContent {
    std::string msgtype;                        // m.text, m.image, m.file, etc.
    std::string body;                           // plain text body
    std::optional<std::string> formatted_body;  // HTML formatted body
    std::optional<std::string> format;          // "org.matrix.custom.html"
    std::optional<std::string> url;             // mxc:// URL for media
    std::optional<nlohmann::json> info;         // media metadata (size, mimetype, etc.)
};

// Member content (for m.room.member events)
struct MemberContent {
    std::string membership; // join, leave, invite, ban, knock
    std::optional<std::string> displayname;
    std::optional<std::string> avatar_url;
};

// Sync response types
struct Timeline {
    std::vector<RoomEvent> events;
    bool limited = false;
    std::optional<std::string> prev_batch;
};

struct RoomState {
    std::vector<RoomEvent> events;
};

struct EphemeralEvents {
    std::vector<RoomEvent> events;
};

// One account-data document as /sync carries it: a type and a content object.
//
// NOT a RoomEvent, and it cannot be one — account data has no sender, no event
// id and no timestamp, because it is not something that happened in a room but
// a value this account stores. The m.direct parser below has always said so in
// a comment; this is the same statement as a type.
//
// The content is raw json rather than a parsed shape on purpose. Account data
// is a key/value store whose values are the CLIENT's, not the protocol's: the
// server stores what was PUT and hands it back untouched, and a type nothing
// on this server interprets must survive the round trip byte for byte. The two
// types that ARE interpreted (m.direct, m.fully_read) are read out of this by
// their consumers.
struct AccountDataEvent {
    std::string type;
    nlohmann::json content;
};

struct JoinedRoom {
    Timeline timeline;
    RoomState state;
    // ROOM account data for the reader — currently m.fully_read, the reader's
    // own read marker, and nothing else.
    //
    // Per-user and per-room, so it is never anybody else's read state: a read
    // RECEIPT, which is the thing other members can see, is a different type
    // in a different section and is deliberately not implemented (see
    // server/docs/read-state.md).
    //
    // Was `std::optional<RoomState>` and unused by either end. RoomState is a
    // vector of RoomEvent, which account data is not — see AccountDataEvent.
    std::vector<AccountDataEvent> account_data;
    std::optional<EphemeralEvents> ephemeral;
    // Serialized as unread_notifications.notification_count — every unread
    // message from somebody else.
    std::optional<int> unread_count;
    // Serialized as unread_notifications.highlight_count — the subset of the
    // above that @-mentions the reader (directly or via @room). A client wants
    // this separately from unread_count so a mention badge can be rendered
    // distinctly from the plain unread dot. Matrix-standard field name.
    std::optional<int> highlight_count;
};

// A room the reader has been invited to and has not joined.
//
// Matrix's shape, and deliberately not JoinedRoom's: no timeline, no
// ephemeral, no unread counts. The reader is not in the room yet, so they get
// what identifies the room and who invited them, and none of its history.
// `invite_state` carries stripped state — the room-level events that name and
// describe the room, plus the member events of the reader and of the inviter.
// SqliteStore::get_invite_state is the exact list; everything else about the
// room (power levels, roles, the rest of the member list, the timeline) is
// withheld until the invite is accepted.
//
// The events are ordinary RoomEvents rather than the spec's four-field
// stripped form: RoomEvent is the one event shape both ends already parse, and
// a state event's id and timestamp are not secrets. What matters — and what is
// enforced in the store — is WHICH events appear here.
struct InvitedRoom {
    RoomState invite_state;
};

// Top-level presence block in a /sync response. Matrix delivers
// m.presence ephemerals here rather than per-room, since presence
// follows the user, not the room. Each event's `sender` is the
// user the presence describes; `content` carries the presence state
// + optional `status_msg`.
struct PresenceEvents {
    std::vector<RoomEvent> events;
};

struct SyncResponse {
    std::string next_batch;
    struct {
        std::map<std::string, JoinedRoom> join;
        // Invites the reader has not answered yet.
        //
        // Every DELIVERED response states the COMPLETE pending set, not a
        // delta — the server restates it the same way it restates m.direct, so
        // that an invite which predates the client's sync token is still
        // learned within one poll instead of never. A client may therefore
        // take a delivered response's `invite` as the current truth and drop
        // an invite that has stopped appearing: that is how an invite accepted
        // or declined on another device clears here, and it is why there is no
        // rooms.leave section to carry the rejection.
        //
        // Absent means the same as empty. A server that predates this section
        // never sends it, and a client then simply never sees an invite — the
        // behaviour it had before.
        std::map<std::string, InvitedRoom> invite;
    } rooms;
    std::optional<PresenceEvents> presence;
    // GLOBAL account data for the reader: every document of theirs that
    // changed within the range this response covers, as stored.
    //
    // A DELTA, unlike the two sections below it, and that is deliberate. The
    // restatement pattern m.direct and the invite list use works because both
    // are small, derived and idempotent; account data is a store the client
    // writes into, of unbounded size, and restating all of it on every poll
    // would put the block list on the wire every 30 seconds forever. Instead
    // each document carries the stream position of its last write, so the
    // sync token the client already holds says which ones it has not seen —
    // see server/src/sync/SyncEngine.cpp and docs/read-state.md.
    //
    // Empty therefore means "nothing changed", NOT "this account has no
    // account data". A client that needs a document it has never been told
    // about reads it from
    // GET /_matrix/client/v3/user/{userId}/account_data/{type}; an initial
    // sync carries the complete set.
    //
    // m.direct is NOT in here. It is derived from rooms.is_direct rather than
    // stored, so it has no stored position to compare a token against, and it
    // keeps the restatement it has always had. On the wire the two share one
    // `account_data.events` array, which is where a client reads both from.
    std::vector<AccountDataEvent> account_data;
    // The reader's direct-message rooms: peer user id -> room ids. Serialized
    // as the Matrix-standard `m.direct` event under top-level
    // account_data.events, and like that event it is a full replacement, not a
    // delta. Absent means "nothing to report this sync", NOT "no DMs".
    //
    // The server derives it from rooms.is_direct rather than storing account
    // data, so it covers both sides of a DM: without it the invited side has
    // no way to tell a DM from a channel, because nothing else in /sync
    // carries the flag.
    std::optional<std::map<std::string, std::vector<std::string>>> direct_rooms;
};

// Messages response (GET /rooms/{roomId}/messages)
struct MessagesResponse {
    std::vector<RoomEvent> chunk;
    std::optional<std::string> start;
    std::optional<std::string> end;
};

// Power levels content (for m.room.power_levels state events)
struct PowerLevelsContent {
    int users_default = 0;
    int events_default = 0;
    int state_default = 50;
    int ban = 50;
    int kick = 50;
    int invite = 0;
    int redact = 50;
    std::map<std::string, int> users;
    std::map<std::string, int> events;
};

// Voice channel marker (state event content for m.room.voice)
struct VoiceChannelContent {
    bool enabled = true;
    int max_participants = 0; // 0 = unlimited
};

// Voice state (state event, state_key = user_id, type = m.call.member)
struct VoiceMemberContent {
    bool active = false;  // true = user is in the voice channel
    bool muted = false;
    bool deafened = false;
    bool screen_sharing = false;
    bool camera_on = false;
    std::string device_id;
    int64_t joined_at = 0;
};

// WebRTC signaling events (timeline events)
struct CallInviteContent {
    std::string call_id;
    int lifetime = 60000; // ms
    struct {
        std::string type; // "offer"
        std::string sdp;
    } offer;
    int version = 1;
};

struct CallAnswerContent {
    std::string call_id;
    struct {
        std::string type; // "answer"
        std::string sdp;
    } answer;
    int version = 1;
};

struct CallCandidatesContent {
    std::string call_id;
    struct Candidate {
        std::string candidate;
        std::string sdpMid;
        int sdpMLineIndex = 0;
    };
    std::vector<Candidate> candidates;
    int version = 1;
};

struct CallHangupContent {
    std::string call_id;
    std::string reason; // "user_hangup", "ice_failed", etc.
    int version = 1;
};

// Category assignment (state event content for bsfchat.room.category)
struct CategoryContent {
    std::string parent_id;  // room ID of the parent category
    int order = 0;          // sort position within category
};

// Room type marker (state event content for bsfchat.room.type)
struct RoomTypeContent {
    std::string type;  // "category", "text", or "voice"
};

// Server role definition. Discord-style: each role carries a bitfield of
// permission flags (see bsfchat/Permissions.h) rather than a numeric level.
struct ServerRole {
    std::string id;             // stable identifier, e.g. "everyone", "mod", "admin", or UUID
    std::string name;           // display name
    std::string color;          // "#RRGGBB"
    int position = 0;           // higher = higher in hierarchy
    std::uint64_t permissions = 0; // bitfield, serialized as hex string
    bool mentionable = false;
    bool hoist = false;         // show members with this role separately in the sidebar
    // Any member may add this role to themselves, and remove it again, without
    // holding MANAGE_ROLES and without anyone outranking anyone. This is what
    // makes an opt-in role picker ("which announcements do you want pinged
    // for?") possible: the rank rules that govern every other role change are
    // unsatisfiable for an ordinary member, who sits at position 0 and can
    // therefore be granted nothing.
    //
    // A role wearing this flag is CONSTRAINED, not merely convenient. The
    // server refuses to store one whose permission bits are not a subset of
    // @everyone's, and refuses to hand one out at claim time if that ever
    // stops holding — see server/src/api/RoleHandler.cpp. Nothing here
    // enforces that; a client must not assume a self-assignable role it reads
    // off the wire is safe to render as harmless without checking its bits.
    bool self_assignable = false;
    // Legacy — kept so older events still parse. Unused by new code.
    int level = 0;
};

// Server roles (state event content for bsfchat.server.roles)
struct ServerRolesContent {
    std::vector<ServerRole> roles;
};

// Per-user role assignment (state event `bsfchat.member.roles`, state_key=userId)
struct MemberRolesContent {
    std::vector<std::string> role_ids;
};

// Per-channel misc settings (state event `bsfchat.channel.settings`, state_key="")
struct ChannelSettingsContent {
    int slowmode_seconds = 0;
};

// Per-channel allow/deny override for a specific role or user.
// state event `bsfchat.channel.permissions`, state_key = "role:<id>" or "user:<mxid>"
struct ChannelPermissionOverride {
    std::uint64_t allow = 0;
    std::uint64_t deny = 0;
};

// Profile
struct UserProfile {
    std::optional<std::string> displayname;
    std::optional<std::string> avatar_url;
};

// JSON serialization
void to_json(nlohmann::json& j, const LoginRequest& r);
void from_json(const nlohmann::json& j, LoginRequest& r);

void to_json(nlohmann::json& j, const LoginResponse& r);
void from_json(const nlohmann::json& j, LoginResponse& r);

void to_json(nlohmann::json& j, const RegisterRequest& r);
void from_json(const nlohmann::json& j, RegisterRequest& r);

void to_json(nlohmann::json& j, const CreateRoomRequest& r);
void from_json(const nlohmann::json& j, CreateRoomRequest& r);

void to_json(nlohmann::json& j, const CreateRoomResponse& r);
void from_json(const nlohmann::json& j, CreateRoomResponse& r);

void to_json(nlohmann::json& j, const RoomEvent& e);
void from_json(const nlohmann::json& j, RoomEvent& e);

void to_json(nlohmann::json& j, const MessageContent& m);
void from_json(const nlohmann::json& j, MessageContent& m);

void to_json(nlohmann::json& j, const MemberContent& m);
void from_json(const nlohmann::json& j, MemberContent& m);

void to_json(nlohmann::json& j, const SyncResponse& r);
void from_json(const nlohmann::json& j, SyncResponse& r);

void to_json(nlohmann::json& j, const MessagesResponse& r);
void from_json(const nlohmann::json& j, MessagesResponse& r);

void to_json(nlohmann::json& j, const UserProfile& p);
void from_json(const nlohmann::json& j, UserProfile& p);

void to_json(nlohmann::json& j, const PowerLevelsContent& p);
void from_json(const nlohmann::json& j, PowerLevelsContent& p);

void to_json(nlohmann::json& j, const VoiceChannelContent& v);
void from_json(const nlohmann::json& j, VoiceChannelContent& v);

void to_json(nlohmann::json& j, const VoiceMemberContent& v);
void from_json(const nlohmann::json& j, VoiceMemberContent& v);

void to_json(nlohmann::json& j, const CallInviteContent& c);
void from_json(const nlohmann::json& j, CallInviteContent& c);

void to_json(nlohmann::json& j, const CallAnswerContent& c);
void from_json(const nlohmann::json& j, CallAnswerContent& c);

void to_json(nlohmann::json& j, const CallCandidatesContent& c);
void from_json(const nlohmann::json& j, CallCandidatesContent& c);

void to_json(nlohmann::json& j, const CallHangupContent& c);
void from_json(const nlohmann::json& j, CallHangupContent& c);

void to_json(nlohmann::json& j, const CategoryContent& c);
void from_json(const nlohmann::json& j, CategoryContent& c);

void to_json(nlohmann::json& j, const RoomTypeContent& r);
void from_json(const nlohmann::json& j, RoomTypeContent& r);

void to_json(nlohmann::json& j, const ServerRole& r);
void from_json(const nlohmann::json& j, ServerRole& r);

void to_json(nlohmann::json& j, const ServerRolesContent& r);
void from_json(const nlohmann::json& j, ServerRolesContent& r);

void to_json(nlohmann::json& j, const MemberRolesContent& r);
void from_json(const nlohmann::json& j, MemberRolesContent& r);

void to_json(nlohmann::json& j, const ChannelSettingsContent& c);
void from_json(const nlohmann::json& j, ChannelSettingsContent& c);

void to_json(nlohmann::json& j, const ChannelPermissionOverride& o);
void from_json(const nlohmann::json& j, ChannelPermissionOverride& o);

} // namespace bsfchat
