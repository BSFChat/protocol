#include "bsfchat/MatrixTypes.h"
#include "bsfchat/Permissions.h"

namespace bsfchat {

namespace {

template<typename T>
void set_optional(nlohmann::json& j, const char* key, const std::optional<T>& val) {
    if (val) j[key] = *val;
}

template<typename T>
void get_optional(const nlohmann::json& j, const char* key, std::optional<T>& val) {
    if (j.contains(key) && !j[key].is_null()) {
        val = j[key].get<T>();
    }
}

} // namespace

// LoginRequest
void to_json(nlohmann::json& j, const LoginRequest& r) {
    j = {{"type", r.type}};
    if (!r.identifier.user.empty()) {
        j["identifier"] = {{"type", r.identifier.type}, {"user", r.identifier.user}};
    }
    if (!r.password.empty()) j["password"] = r.password;
    if (!r.token.empty()) j["token"] = r.token;
    set_optional(j, "device_id", r.device_id);
    set_optional(j, "initial_device_display_name", r.initial_device_display_name);
}

void from_json(const nlohmann::json& j, LoginRequest& r) {
    r.type = j.at("type").get<std::string>();
    if (j.contains("identifier")) {
        r.identifier.type = j["identifier"].value("type", "m.id.user");
        r.identifier.user = j["identifier"].value("user", "");
    }
    r.password = j.value("password", "");
    r.token = j.value("token", "");
    get_optional(j, "device_id", r.device_id);
    get_optional(j, "initial_device_display_name", r.initial_device_display_name);
}

// LoginResponse
void to_json(nlohmann::json& j, const LoginResponse& r) {
    j = {
        {"user_id", r.user_id},
        {"access_token", r.access_token},
        {"device_id", r.device_id},
    };
}

void from_json(const nlohmann::json& j, LoginResponse& r) {
    r.user_id = j.at("user_id").get<std::string>();
    r.access_token = j.at("access_token").get<std::string>();
    r.device_id = j.at("device_id").get<std::string>();
}

// RegisterRequest
void to_json(nlohmann::json& j, const RegisterRequest& r) {
    j = {{"username", r.username}, {"password", r.password}};
    set_optional(j, "device_id", r.device_id);
    set_optional(j, "initial_device_display_name", r.initial_device_display_name);
}

void from_json(const nlohmann::json& j, RegisterRequest& r) {
    r.username = j.at("username").get<std::string>();
    r.password = j.at("password").get<std::string>();
    get_optional(j, "device_id", r.device_id);
    get_optional(j, "initial_device_display_name", r.initial_device_display_name);
}

// CreateRoomRequest
void to_json(nlohmann::json& j, const CreateRoomRequest& r) {
    j = nlohmann::json::object();
    set_optional(j, "name", r.name);
    set_optional(j, "topic", r.topic);
    set_optional(j, "room_alias_name", r.room_alias_name);
    j["visibility"] = r.visibility;
    if (!r.invite.empty()) j["invite"] = r.invite;
    set_optional(j, "preset", r.preset);
    set_optional(j, "is_category", r.is_category);
    set_optional(j, "parent_id", r.parent_id);
    set_optional(j, "sort_order", r.sort_order);
    set_optional(j, "is_direct", r.is_direct);
}

void from_json(const nlohmann::json& j, CreateRoomRequest& r) {
    get_optional(j, "name", r.name);
    get_optional(j, "topic", r.topic);
    get_optional(j, "room_alias_name", r.room_alias_name);
    r.visibility = j.value("visibility", "private");
    if (j.contains("invite")) r.invite = j["invite"].get<std::vector<std::string>>();
    get_optional(j, "preset", r.preset);
    get_optional(j, "is_category", r.is_category);
    get_optional(j, "parent_id", r.parent_id);
    get_optional(j, "sort_order", r.sort_order);
    get_optional(j, "is_direct", r.is_direct);
}

// CreateRoomResponse
void to_json(nlohmann::json& j, const CreateRoomResponse& r) {
    j = {{"room_id", r.room_id}};
}

void from_json(const nlohmann::json& j, CreateRoomResponse& r) {
    r.room_id = j.at("room_id").get<std::string>();
}

// RoomEvent
void to_json(nlohmann::json& j, const RoomEvent& e) {
    j = {
        {"event_id", e.event_id},
        {"room_id", e.room_id},
        {"sender", e.sender},
        {"type", e.type},
        {"content", e.content.data},
        {"origin_server_ts", e.origin_server_ts},
    };
    if (e.state_key) j["state_key"] = *e.state_key;
    if (e.unsigned_data) j["unsigned"] = e.unsigned_data->data;
}

void from_json(const nlohmann::json& j, RoomEvent& e) {
    e.event_id = j.value("event_id", "");
    e.room_id = j.value("room_id", "");
    e.sender = j.at("sender").get<std::string>();
    e.type = j.at("type").get<std::string>();
    e.content.data = j.value("content", nlohmann::json::object());
    e.origin_server_ts = j.value("origin_server_ts", int64_t(0));
    get_optional(j, "state_key", e.state_key);
    if (j.contains("unsigned")) {
        e.unsigned_data = EventContent{.data = j["unsigned"]};
    }
}

// MessageContent
void to_json(nlohmann::json& j, const MessageContent& m) {
    j = {{"msgtype", m.msgtype}, {"body", m.body}};
    set_optional(j, "formatted_body", m.formatted_body);
    set_optional(j, "format", m.format);
    set_optional(j, "url", m.url);
    if (m.info) j["info"] = *m.info;
}

void from_json(const nlohmann::json& j, MessageContent& m) {
    m.msgtype = j.at("msgtype").get<std::string>();
    m.body = j.at("body").get<std::string>();
    get_optional(j, "formatted_body", m.formatted_body);
    get_optional(j, "format", m.format);
    get_optional(j, "url", m.url);
    if (j.contains("info")) m.info = j["info"];
}

// MemberContent
void to_json(nlohmann::json& j, const MemberContent& m) {
    j = {{"membership", m.membership}};
    set_optional(j, "displayname", m.displayname);
    set_optional(j, "avatar_url", m.avatar_url);
}

void from_json(const nlohmann::json& j, MemberContent& m) {
    m.membership = j.at("membership").get<std::string>();
    get_optional(j, "displayname", m.displayname);
    get_optional(j, "avatar_url", m.avatar_url);
}

// SyncResponse
void to_json(nlohmann::json& j, const SyncResponse& r) {
    j = {{"next_batch", r.next_batch}};

    auto rooms_join = nlohmann::json::object();
    for (const auto& [room_id, room] : r.rooms.join) {
        nlohmann::json room_json;

        // Timeline
        nlohmann::json timeline;
        timeline["events"] = nlohmann::json::array();
        for (const auto& event : room.timeline.events) {
            nlohmann::json ev;
            to_json(ev, event);
            timeline["events"].push_back(ev);
        }
        timeline["limited"] = room.timeline.limited;
        if (room.timeline.prev_batch) timeline["prev_batch"] = *room.timeline.prev_batch;
        room_json["timeline"] = timeline;

        // State
        nlohmann::json state;
        state["events"] = nlohmann::json::array();
        for (const auto& event : room.state.events) {
            nlohmann::json ev;
            to_json(ev, event);
            state["events"].push_back(ev);
        }
        room_json["state"] = state;

        // Ephemeral events (typing, etc.)
        if (room.ephemeral && !room.ephemeral->events.empty()) {
            nlohmann::json ephemeral;
            ephemeral["events"] = nlohmann::json::array();
            for (const auto& event : room.ephemeral->events) {
                nlohmann::json ev;
                to_json(ev, event);
                ephemeral["events"].push_back(ev);
            }
            room_json["ephemeral"] = ephemeral;
        }

        // Unread notification count (bsfchat extension)
        if (room.unread_count) {
            room_json["unread_notifications"] = {
                {"notification_count", *room.unread_count}
            };
        }

        rooms_join[room_id] = room_json;
    }
    j["rooms"] = {{"join", rooms_join}};
}

void from_json(const nlohmann::json& j, SyncResponse& r) {
    r.next_batch = j.at("next_batch").get<std::string>();

    if (j.contains("rooms") && j["rooms"].contains("join")) {
        for (auto& [room_id, room_json] : j["rooms"]["join"].items()) {
            JoinedRoom room;

            if (room_json.contains("timeline")) {
                auto& tl = room_json["timeline"];
                if (tl.contains("events")) {
                    for (auto& ev_json : tl["events"]) {
                        RoomEvent ev;
                        from_json(ev_json, ev);
                        room.timeline.events.push_back(std::move(ev));
                    }
                }
                room.timeline.limited = tl.value("limited", false);
                if (tl.contains("prev_batch")) {
                    room.timeline.prev_batch = tl["prev_batch"].get<std::string>();
                }
            }

            if (room_json.contains("state") && room_json["state"].contains("events")) {
                for (auto& ev_json : room_json["state"]["events"]) {
                    RoomEvent ev;
                    from_json(ev_json, ev);
                    room.state.events.push_back(std::move(ev));
                }
            }

            if (room_json.contains("ephemeral") && room_json["ephemeral"].contains("events")) {
                room.ephemeral = EphemeralEvents{};
                for (auto& ev_json : room_json["ephemeral"]["events"]) {
                    RoomEvent ev;
                    from_json(ev_json, ev);
                    room.ephemeral->events.push_back(std::move(ev));
                }
            }

            if (room_json.contains("unread_notifications")) {
                const auto& un = room_json["unread_notifications"];
                if (un.contains("notification_count")) {
                    room.unread_count = un["notification_count"].get<int>();
                }
            }

            r.rooms.join[room_id] = std::move(room);
        }
    }
}

// MessagesResponse
void to_json(nlohmann::json& j, const MessagesResponse& r) {
    j = nlohmann::json::object();
    j["chunk"] = nlohmann::json::array();
    for (const auto& event : r.chunk) {
        nlohmann::json ev;
        to_json(ev, event);
        j["chunk"].push_back(ev);
    }
    set_optional(j, "start", r.start);
    set_optional(j, "end", r.end);
}

void from_json(const nlohmann::json& j, MessagesResponse& r) {
    if (j.contains("chunk")) {
        for (auto& ev_json : j["chunk"]) {
            RoomEvent ev;
            from_json(ev_json, ev);
            r.chunk.push_back(std::move(ev));
        }
    }
    get_optional(j, "start", r.start);
    get_optional(j, "end", r.end);
}

// UserProfile
void to_json(nlohmann::json& j, const UserProfile& p) {
    j = nlohmann::json::object();
    set_optional(j, "displayname", p.displayname);
    set_optional(j, "avatar_url", p.avatar_url);
}

void from_json(const nlohmann::json& j, UserProfile& p) {
    get_optional(j, "displayname", p.displayname);
    get_optional(j, "avatar_url", p.avatar_url);
}

// PowerLevelsContent
void to_json(nlohmann::json& j, const PowerLevelsContent& p) {
    j = {
        {"users_default", p.users_default},
        {"events_default", p.events_default},
        {"state_default", p.state_default},
        {"ban", p.ban},
        {"kick", p.kick},
        {"invite", p.invite},
        {"redact", p.redact},
    };
    if (!p.users.empty()) j["users"] = p.users;
    if (!p.events.empty()) j["events"] = p.events;
}

void from_json(const nlohmann::json& j, PowerLevelsContent& p) {
    p.users_default = j.value("users_default", 0);
    p.events_default = j.value("events_default", 0);
    p.state_default = j.value("state_default", 50);
    p.ban = j.value("ban", 50);
    p.kick = j.value("kick", 50);
    p.invite = j.value("invite", 0);
    p.redact = j.value("redact", 50);
    if (j.contains("users") && j["users"].is_object()) {
        p.users = j["users"].get<std::map<std::string, int>>();
    }
    if (j.contains("events") && j["events"].is_object()) {
        p.events = j["events"].get<std::map<std::string, int>>();
    }
}

// VoiceChannelContent
void to_json(nlohmann::json& j, const VoiceChannelContent& v) {
    j = {{"enabled", v.enabled}, {"max_participants", v.max_participants}};
}

void from_json(const nlohmann::json& j, VoiceChannelContent& v) {
    v.enabled = j.value("enabled", true);
    v.max_participants = j.value("max_participants", 0);
}

// VoiceMemberContent
void to_json(nlohmann::json& j, const VoiceMemberContent& v) {
    j = {
        {"active", v.active},
        {"muted", v.muted},
        {"deafened", v.deafened},
        {"device_id", v.device_id},
        {"joined_at", v.joined_at},
    };
}

void from_json(const nlohmann::json& j, VoiceMemberContent& v) {
    v.active = j.value("active", false);
    v.muted = j.value("muted", false);
    v.deafened = j.value("deafened", false);
    v.device_id = j.value("device_id", "");
    v.joined_at = j.value("joined_at", int64_t(0));
}

// CallInviteContent
void to_json(nlohmann::json& j, const CallInviteContent& c) {
    j = {
        {"call_id", c.call_id},
        {"lifetime", c.lifetime},
        {"offer", {{"type", c.offer.type}, {"sdp", c.offer.sdp}}},
        {"version", c.version},
    };
}

void from_json(const nlohmann::json& j, CallInviteContent& c) {
    c.call_id = j.at("call_id").get<std::string>();
    c.lifetime = j.value("lifetime", 60000);
    if (j.contains("offer")) {
        c.offer.type = j["offer"].value("type", "");
        c.offer.sdp = j["offer"].value("sdp", "");
    }
    c.version = j.value("version", 1);
}

// CallAnswerContent
void to_json(nlohmann::json& j, const CallAnswerContent& c) {
    j = {
        {"call_id", c.call_id},
        {"answer", {{"type", c.answer.type}, {"sdp", c.answer.sdp}}},
        {"version", c.version},
    };
}

void from_json(const nlohmann::json& j, CallAnswerContent& c) {
    c.call_id = j.at("call_id").get<std::string>();
    if (j.contains("answer")) {
        c.answer.type = j["answer"].value("type", "");
        c.answer.sdp = j["answer"].value("sdp", "");
    }
    c.version = j.value("version", 1);
}

// CallCandidatesContent
void to_json(nlohmann::json& j, const CallCandidatesContent& c) {
    j = {{"call_id", c.call_id}, {"version", c.version}};
    j["candidates"] = nlohmann::json::array();
    for (const auto& cand : c.candidates) {
        j["candidates"].push_back({
            {"candidate", cand.candidate},
            {"sdpMid", cand.sdpMid},
            {"sdpMLineIndex", cand.sdpMLineIndex},
        });
    }
}

void from_json(const nlohmann::json& j, CallCandidatesContent& c) {
    c.call_id = j.at("call_id").get<std::string>();
    c.version = j.value("version", 1);
    if (j.contains("candidates")) {
        for (const auto& cj : j["candidates"]) {
            CallCandidatesContent::Candidate cand;
            cand.candidate = cj.value("candidate", "");
            cand.sdpMid = cj.value("sdpMid", "");
            cand.sdpMLineIndex = cj.value("sdpMLineIndex", 0);
            c.candidates.push_back(std::move(cand));
        }
    }
}

// CallHangupContent
void to_json(nlohmann::json& j, const CallHangupContent& c) {
    j = {
        {"call_id", c.call_id},
        {"reason", c.reason},
        {"version", c.version},
    };
}

void from_json(const nlohmann::json& j, CallHangupContent& c) {
    c.call_id = j.at("call_id").get<std::string>();
    c.reason = j.value("reason", "");
    c.version = j.value("version", 1);
}

// CategoryContent
void to_json(nlohmann::json& j, const CategoryContent& c) {
    j = {{"parent_id", c.parent_id}, {"order", c.order}};
}

void from_json(const nlohmann::json& j, CategoryContent& c) {
    c.parent_id = j.value("parent_id", "");
    c.order = j.value("order", 0);
}

// RoomTypeContent
void to_json(nlohmann::json& j, const RoomTypeContent& r) {
    j = {{"type", r.type}};
}

void from_json(const nlohmann::json& j, RoomTypeContent& r) {
    r.type = j.value("type", "");
}

// ServerRole
void to_json(nlohmann::json& j, const ServerRole& r) {
    j = {
        {"id", r.id},
        {"name", r.name},
        {"color", r.color},
        {"position", r.position},
        {"permissions", permission::flags_to_hex(r.permissions)},
        {"mentionable", r.mentionable},
        {"hoist", r.hoist}
    };
    // Legacy
    if (r.level != 0) j["level"] = r.level;
}

void from_json(const nlohmann::json& j, ServerRole& r) {
    r.id = j.value("id", "");
    r.name = j.value("name", "");
    r.color = j.value("color", "");
    r.position = j.value("position", 0);
    if (j.contains("permissions")) {
        const auto& p = j["permissions"];
        if (p.is_string()) {
            r.permissions = permission::flags_from_hex(p.get<std::string>());
        } else if (p.is_number_integer()) {
            r.permissions = p.get<std::uint64_t>();
        }
    }
    r.mentionable = j.value("mentionable", false);
    r.hoist = j.value("hoist", false);
    r.level = j.value("level", 0);
    // Fallback: if role has no explicit id but has a name, derive a stable id.
    if (r.id.empty() && !r.name.empty()) r.id = r.name;
}

// ServerRolesContent
void to_json(nlohmann::json& j, const ServerRolesContent& r) {
    j = nlohmann::json::object();
    j["roles"] = nlohmann::json::array();
    for (const auto& role : r.roles) {
        nlohmann::json rj;
        to_json(rj, role);
        j["roles"].push_back(rj);
    }
}

void from_json(const nlohmann::json& j, ServerRolesContent& r) {
    if (j.contains("roles") && j["roles"].is_array()) {
        for (const auto& rj : j["roles"]) {
            ServerRole role;
            from_json(rj, role);
            r.roles.push_back(std::move(role));
        }
    }
}

// MemberRolesContent
void to_json(nlohmann::json& j, const MemberRolesContent& r) {
    j = nlohmann::json::object();
    j["role_ids"] = r.role_ids;
}

void from_json(const nlohmann::json& j, MemberRolesContent& r) {
    if (j.contains("role_ids") && j["role_ids"].is_array()) {
        for (const auto& id : j["role_ids"]) {
            if (id.is_string()) r.role_ids.push_back(id.get<std::string>());
        }
    }
}

// ChannelSettingsContent
void to_json(nlohmann::json& j, const ChannelSettingsContent& c) {
    j = nlohmann::json::object();
    j["slowmode_seconds"] = c.slowmode_seconds;
}

void from_json(const nlohmann::json& j, ChannelSettingsContent& c) {
    c.slowmode_seconds = j.value("slowmode_seconds", 0);
}

// ChannelPermissionOverride
void to_json(nlohmann::json& j, const ChannelPermissionOverride& o) {
    j = nlohmann::json::object();
    j["allow"] = permission::flags_to_hex(o.allow);
    j["deny"] = permission::flags_to_hex(o.deny);
}

void from_json(const nlohmann::json& j, ChannelPermissionOverride& o) {
    auto read = [&](const char* key, std::uint64_t& out) {
        if (!j.contains(key)) return;
        const auto& v = j[key];
        if (v.is_string()) out = permission::flags_from_hex(v.get<std::string>());
        else if (v.is_number_integer()) out = v.get<std::uint64_t>();
    };
    read("allow", o.allow);
    read("deny", o.deny);
}

} // namespace bsfchat
