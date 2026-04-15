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
};

struct LoginResponse {
    std::string user_id;
    std::string access_token;
    std::string device_id;
};

// Registration
struct RegisterRequest {
    std::string username;
    std::string password;
    std::optional<std::string> device_id;
    std::optional<std::string> initial_device_display_name;
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

struct JoinedRoom {
    Timeline timeline;
    RoomState state;
    std::optional<RoomState> account_data;
    std::optional<EphemeralEvents> ephemeral;
    std::optional<int> unread_count;
};

struct SyncResponse {
    std::string next_batch;
    struct {
        std::map<std::string, JoinedRoom> join;
    } rooms;
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

// Server role definition
struct ServerRole {
    std::string name;
    int level = 0;
    std::string color;
};

// Server roles (state event content for bsfchat.server.roles)
struct ServerRolesContent {
    std::vector<ServerRole> roles;
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

} // namespace bsfchat
