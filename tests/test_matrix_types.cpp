#include <gtest/gtest.h>
#include "bsfchat/Constants.h"
#include "bsfchat/MatrixTypes.h"
#include "bsfchat/Permissions.h"

#include <cstdint>
#include <limits>

using namespace bsfchat;
using json = nlohmann::json;

TEST(LoginRequest, RoundTrip) {
    LoginRequest req;
    req.type = "m.login.password";
    req.identifier.type = "m.id.user";
    req.identifier.user = "alice";
    req.password = "secret";
    req.device_id = "MYDEVICE";

    json j;
    to_json(j, req);

    EXPECT_EQ(j["type"], "m.login.password");
    EXPECT_EQ(j["identifier"]["user"], "alice");
    EXPECT_EQ(j["password"], "secret");
    EXPECT_EQ(j["device_id"], "MYDEVICE");

    LoginRequest parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.type, "m.login.password");
    EXPECT_EQ(parsed.identifier.user, "alice");
    EXPECT_EQ(parsed.password, "secret");
    EXPECT_EQ(parsed.device_id, "MYDEVICE");
}

TEST(LoginResponse, RoundTrip) {
    LoginResponse resp{
        .user_id = "@alice:example.com",
        .access_token = "token123",
        .device_id = "DEVICE1",
    };

    json j;
    to_json(j, resp);

    LoginResponse parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.user_id, resp.user_id);
    EXPECT_EQ(parsed.access_token, resp.access_token);
    EXPECT_EQ(parsed.device_id, resp.device_id);
}

TEST(RegisterRequest, RoundTrip) {
    RegisterRequest req{.username = "bob", .password = "password123"};

    json j;
    to_json(j, req);
    EXPECT_EQ(j["username"], "bob");

    RegisterRequest parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.username, "bob");
    EXPECT_EQ(parsed.password, "password123");
}

TEST(CreateRoomRequest, RoundTrip) {
    CreateRoomRequest req;
    req.name = "General";
    req.topic = "General discussion";
    req.visibility = "public";
    req.invite = {"@bob:example.com"};

    json j;
    to_json(j, req);
    EXPECT_EQ(j["name"], "General");
    EXPECT_EQ(j["visibility"], "public");
    EXPECT_EQ(j["invite"].size(), 1u);

    CreateRoomRequest parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.name, "General");
    EXPECT_EQ(parsed.visibility, "public");
    EXPECT_EQ(parsed.invite.size(), 1u);
}

TEST(RoomEvent, RoundTrip) {
    RoomEvent event;
    event.event_id = "$abc:example.com";
    event.room_id = "!room:example.com";
    event.sender = "@alice:example.com";
    event.type = "m.room.message";
    event.content.data = {{"msgtype", "m.text"}, {"body", "Hello"}};
    event.origin_server_ts = 1700000000000;

    json j;
    to_json(j, event);

    RoomEvent parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.event_id, event.event_id);
    EXPECT_EQ(parsed.sender, event.sender);
    EXPECT_EQ(parsed.type, event.type);
    EXPECT_EQ(parsed.content.data["body"], "Hello");
    EXPECT_EQ(parsed.origin_server_ts, event.origin_server_ts);
}

TEST(RoomEvent, StateEvent) {
    RoomEvent event;
    event.event_id = "$abc:example.com";
    event.room_id = "!room:example.com";
    event.sender = "@alice:example.com";
    event.type = "m.room.name";
    event.state_key = "";
    event.content.data = {{"name", "General"}};
    event.origin_server_ts = 1700000000000;

    json j;
    to_json(j, event);
    EXPECT_TRUE(j.contains("state_key"));

    RoomEvent parsed;
    from_json(j, parsed);
    EXPECT_TRUE(parsed.state_key.has_value());
    EXPECT_EQ(*parsed.state_key, "");
}

TEST(MessageContent, RoundTrip) {
    MessageContent msg;
    msg.msgtype = "m.text";
    msg.body = "Hello world";
    msg.format = "org.matrix.custom.html";
    msg.formatted_body = "<b>Hello world</b>";

    json j;
    to_json(j, msg);

    MessageContent parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.msgtype, "m.text");
    EXPECT_EQ(parsed.body, "Hello world");
    EXPECT_EQ(parsed.formatted_body, "<b>Hello world</b>");
}

TEST(MemberContent, RoundTrip) {
    MemberContent member;
    member.membership = "join";
    member.displayname = "Alice";

    json j;
    to_json(j, member);

    MemberContent parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.membership, "join");
    EXPECT_EQ(parsed.displayname, "Alice");
}

TEST(SyncResponse, RoundTrip) {
    SyncResponse sync;
    sync.next_batch = "s42";

    JoinedRoom room;
    RoomEvent msg;
    msg.event_id = "$1:example.com";
    msg.room_id = "!room:example.com";
    msg.sender = "@alice:example.com";
    msg.type = "m.room.message";
    msg.content.data = {{"msgtype", "m.text"}, {"body", "hi"}};
    msg.origin_server_ts = 1700000000000;
    room.timeline.events.push_back(msg);

    RoomEvent state_ev;
    state_ev.event_id = "$2:example.com";
    state_ev.room_id = "!room:example.com";
    state_ev.sender = "@alice:example.com";
    state_ev.type = "m.room.name";
    state_ev.state_key = "";
    state_ev.content.data = {{"name", "Test Room"}};
    state_ev.origin_server_ts = 1700000000000;
    room.state.events.push_back(state_ev);

    sync.rooms.join["!room:example.com"] = room;

    json j;
    to_json(j, sync);

    SyncResponse parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.next_batch, "s42");
    ASSERT_EQ(parsed.rooms.join.count("!room:example.com"), 1u);
    EXPECT_EQ(parsed.rooms.join["!room:example.com"].timeline.events.size(), 1u);
    EXPECT_EQ(parsed.rooms.join["!room:example.com"].state.events.size(), 1u);
}

// m.direct rides in top-level account_data. It is what lets the invited side
// of a DM recognise the room; nothing else in /sync carries the flag.
TEST(SyncResponse, DirectRoomsRoundTripAsMDirectAccountData) {
    SyncResponse sync;
    sync.next_batch = "s1";
    sync.direct_rooms.emplace()["@bob:example.com"] = {"!dm:example.com"};

    json j;
    to_json(j, sync);
    ASSERT_EQ(j["account_data"]["events"].size(), 1u);
    EXPECT_EQ(j["account_data"]["events"][0]["type"], "m.direct");
    EXPECT_EQ(j["account_data"]["events"][0]["content"]["@bob:example.com"][0],
              "!dm:example.com");

    SyncResponse parsed;
    from_json(j, parsed);
    ASSERT_TRUE(parsed.direct_rooms.has_value());
    EXPECT_EQ(parsed.direct_rooms->at("@bob:example.com"),
              std::vector<std::string>{"!dm:example.com"});
}

// Absent must stay distinguishable from empty: absent is "nothing to report",
// and a client that read it as "no DMs" would drop its list on every sync.
TEST(SyncResponse, DirectRoomsAbsentStaysAbsentAndMalformedIsSkipped) {
    SyncResponse sync;
    sync.next_batch = "s1";
    json j;
    to_json(j, sync);
    EXPECT_FALSE(j.contains("account_data"));

    SyncResponse parsed;
    from_json(j, parsed);
    EXPECT_FALSE(parsed.direct_rooms.has_value());

    j["account_data"] = {{"events", json::array({
        json{{"type", "m.push_rules"}, {"content", json::object()}},
        json{{"type", "m.direct"}, {"content", {{"@bob:example.com", "not-an-array"},
                                                {"@eve:example.com", json::array({"!r:x", 7})}}}},
    })}};
    SyncResponse lenient;
    from_json(j, lenient);
    ASSERT_TRUE(lenient.direct_rooms.has_value());
    EXPECT_EQ(lenient.direct_rooms->count("@bob:example.com"), 0u);
    EXPECT_EQ(lenient.direct_rooms->at("@eve:example.com"),
              std::vector<std::string>{"!r:x"});
}

TEST(SyncResponse, InviteSectionRoundTrip) {
    SyncResponse sync;
    sync.next_batch = "s7";

    InvitedRoom invited;
    RoomEvent name_ev;
    name_ev.event_id = "$n:example.com";
    name_ev.room_id = "!secret:example.com";
    name_ev.sender = "@alice:example.com";
    name_ev.type = "m.room.name";
    name_ev.state_key = "";
    name_ev.content.data = {{"name", "Planning"}};
    name_ev.origin_server_ts = 1700000000000;
    invited.invite_state.events.push_back(name_ev);

    RoomEvent member_ev;
    member_ev.event_id = "$m:example.com";
    member_ev.room_id = "!secret:example.com";
    member_ev.sender = "@alice:example.com";
    member_ev.type = "m.room.member";
    member_ev.state_key = "@bob:example.com";
    member_ev.content.data = {{"membership", "invite"}};
    member_ev.origin_server_ts = 1700000000001;
    invited.invite_state.events.push_back(member_ev);

    sync.rooms.invite["!secret:example.com"] = invited;

    json j;
    to_json(j, sync);
    // Matrix's shape: rooms.invite.<id>.invite_state.events — NOT a timeline.
    ASSERT_TRUE(j["rooms"].contains("invite"));
    auto& room_json = j["rooms"]["invite"]["!secret:example.com"];
    ASSERT_EQ(room_json["invite_state"]["events"].size(), 2u);
    EXPECT_FALSE(room_json.contains("timeline"));
    EXPECT_EQ(room_json["invite_state"]["events"][1]["content"]["membership"], "invite");

    SyncResponse parsed;
    from_json(j, parsed);
    ASSERT_EQ(parsed.rooms.invite.count("!secret:example.com"), 1u);
    auto& events = parsed.rooms.invite["!secret:example.com"].invite_state.events;
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].type, "m.room.name");
    EXPECT_EQ(events[1].state_key, "@bob:example.com");
}

TEST(SyncResponse, NoInvitesWritesNoInviteSection) {
    // A server with nothing to report must send the bytes it always did: an
    // always-present empty `invite` object would be new output on every poll
    // of every existing deployment.
    SyncResponse sync;
    sync.next_batch = "s1";
    sync.rooms.join["!room:example.com"] = JoinedRoom{};

    json j;
    to_json(j, sync);
    EXPECT_FALSE(j["rooms"].contains("invite"));
    EXPECT_TRUE(j["rooms"].contains("join"));

    SyncResponse parsed;
    from_json(j, parsed);
    EXPECT_TRUE(parsed.rooms.invite.empty());
}

TEST(SyncResponse, InviteWithEmptyStateIsStillAnInvite) {
    // The room id is the invitation. A client must not drop the entry just
    // because the server had no nameable state to strip for it.
    json j = {
        {"next_batch", "s3"},
        {"rooms", {{"invite", {{"!bare:example.com", json::object()}}}}},
    };

    SyncResponse parsed;
    from_json(j, parsed);
    ASSERT_EQ(parsed.rooms.invite.count("!bare:example.com"), 1u);
    EXPECT_TRUE(parsed.rooms.invite["!bare:example.com"].invite_state.events.empty());
}

TEST(MessagesResponse, RoundTrip) {
    MessagesResponse resp;
    resp.start = "t1";
    resp.end = "t2";

    RoomEvent ev;
    ev.event_id = "$1:example.com";
    ev.room_id = "!room:example.com";
    ev.sender = "@alice:example.com";
    ev.type = "m.room.message";
    ev.content.data = {{"msgtype", "m.text"}, {"body", "test"}};
    ev.origin_server_ts = 1700000000000;
    resp.chunk.push_back(ev);

    json j;
    to_json(j, resp);

    MessagesResponse parsed;
    from_json(j, parsed);
    EXPECT_EQ(parsed.start, "t1");
    EXPECT_EQ(parsed.end, "t2");
    EXPECT_EQ(parsed.chunk.size(), 1u);
}

// A role event written before `self_assignable` existed must parse as NOT
// self-assignable. The field is the whole authority for "any member may take
// this", so a missing key defaulting the other way would make every role on
// every upgraded deployment claimable by everyone at once.
TEST(ServerRoleSerialization, SelfAssignableDefaultsFalseWhenAbsent) {
    json legacy = {
        {"id", "mod"}, {"name", "Moderator"}, {"color", "#43b581"},
        {"position", 10}, {"permissions", "0x1f"}, {"hoist", true}
    };
    ServerRole parsed;
    from_json(legacy, parsed);
    EXPECT_FALSE(parsed.self_assignable);
    EXPECT_EQ(parsed.position, 10);
}

// And it survives a round trip, in both states, so a role edit that leaves the
// flag alone cannot silently clear it.
TEST(ServerRoleSerialization, SelfAssignableRoundTrips) {
    for (bool flag : {false, true}) {
        ServerRole r;
        r.id = "notify-boss";
        r.name = "Boss pings";
        r.position = 1;
        r.permissions = 0;
        r.self_assignable = flag;

        json j;
        to_json(j, r);
        EXPECT_EQ(j["self_assignable"], flag);

        ServerRole back;
        from_json(j, back);
        EXPECT_EQ(back.self_assignable, flag);
    }
}

// ── Account data ─────────────────────────────────────────────────────────

// Global account data goes out beside m.direct in the one `account_data`
// array, and comes back as documents. The whole point of the section is that
// a second device learns about a write it did not make, so the content has to
// survive the trip unaltered — including keys this protocol has never heard
// of, since account data is a store the CLIENT owns.
TEST(SyncResponse, GlobalAccountDataRoundTrips) {
    SyncResponse sync;
    sync.next_batch = "s1";
    sync.account_data.push_back(
        {"m.ignored_user_list",
         json{{"ignored_users", {{"@spammer:example.com", json::object()}}},
              {"some.future.key", 7}}});
    sync.direct_rooms.emplace()["@bob:example.com"] = {"!dm:example.com"};

    json j;
    to_json(j, sync);
    ASSERT_EQ(j["account_data"]["events"].size(), 2u);

    SyncResponse parsed;
    from_json(j, parsed);
    // m.direct reaches BOTH: the raw document list, and direct_rooms, where
    // every existing reader of it looks.
    ASSERT_EQ(parsed.account_data.size(), 2u);
    EXPECT_EQ(parsed.account_data[0].type, "m.ignored_user_list");
    EXPECT_EQ(parsed.account_data[0].content["some.future.key"], 7);
    EXPECT_EQ(parsed.account_data[0]
                  .content["ignored_users"]
                  .count("@spammer:example.com"),
              1u);
    EXPECT_EQ(parsed.account_data[1].type, "m.direct");
    ASSERT_TRUE(parsed.direct_rooms.has_value());
    EXPECT_EQ(parsed.direct_rooms->at("@bob:example.com"),
              std::vector<std::string>{"!dm:example.com"});
}

// Empty means "nothing changed", and must put nothing on the wire: an
// account_data section appearing on every idle poll is bytes per client per
// 30 seconds, and a client cannot tell a restatement from a change.
TEST(SyncResponse, NoAccountDataWritesNoSection) {
    SyncResponse sync;
    sync.next_batch = "s1";
    json j;
    to_json(j, sync);
    EXPECT_FALSE(j.contains("account_data"));
}

// The read marker, in room account data. `event_id` is the spec's field;
// the timestamp beside it is what this client's unread dot is arithmetic on
// (client/src/core/ReadState.h), and it travels so that the dot never needs a
// /messages request to resolve the id.
TEST(SyncResponse, RoomAccountDataCarriesTheReadMarker) {
    SyncResponse sync;
    sync.next_batch = "s2";
    JoinedRoom room;
    room.account_data.push_back({std::string(event_type::kFullyRead),
                                 json{{std::string(fully_read::kEventId), "$read:example.com"},
                                      {std::string(fully_read::kOriginServerTs), 1700000000000}}});
    sync.rooms.join["!room:example.com"] = std::move(room);

    json j;
    to_json(j, sync);
    const auto& events = j["rooms"]["join"]["!room:example.com"]["account_data"]["events"];
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0]["type"], "m.fully_read");
    EXPECT_EQ(events[0]["content"]["event_id"], "$read:example.com");
    EXPECT_EQ(events[0]["content"]["bsfchat.origin_server_ts"], 1700000000000);

    SyncResponse parsed;
    from_json(j, parsed);
    const auto& back = parsed.rooms.join.at("!room:example.com").account_data;
    ASSERT_EQ(back.size(), 1u);
    EXPECT_EQ(back[0].type, "m.fully_read");
    EXPECT_EQ(back[0].content[std::string(fully_read::kOriginServerTs)].get<int64_t>(),
              1700000000000);
}

// A room with no marker sends no section at all — an upgraded server must
// look exactly like the old one for every room nobody has read.
TEST(SyncResponse, RoomWithoutAccountDataWritesNoSection) {
    SyncResponse sync;
    sync.next_batch = "s2";
    sync.rooms.join["!room:example.com"] = JoinedRoom{};
    json j;
    to_json(j, sync);
    EXPECT_FALSE(j["rooms"]["join"]["!room:example.com"].contains("account_data"));
}

// One malformed document must not cost the client everything after it. An
// account-data array is the one place on this endpoint where the contents are
// whatever some other client wrote.
TEST(SyncResponse, MalformedAccountDataEntriesAreSkippedNotFatal) {
    json j = {{"next_batch", "s3"},
              {"account_data",
               {{"events", json::array({
                                json::array({1, 2}),                    // not an object
                                json{{"content", json::object()}},      // no type
                                json{{"type", "m.x"}},                  // no content
                                json{{"type", "m.x"}, {"content", 7}},  // content not an object
                                json{{"type", "m.good"}, {"content", {{"k", "v"}}}},
                            })}}}};
    SyncResponse parsed;
    from_json(j, parsed);
    ASSERT_EQ(parsed.account_data.size(), 1u);
    EXPECT_EQ(parsed.account_data[0].type, "m.good");
}

// ---------------------------------------------------------------------------
// Permission values from the wire (audit 2026-11-15, P-1 / P-2)
// ---------------------------------------------------------------------------
// is_number_integer() is true for negative numbers, and get<uint64_t>() of -1
// is all ones — every flag, ADMINISTRATOR included. Any account able to write
// a roles or override event could hand itself everything with `-1`.

namespace {
ServerRole role_with_permissions(const json& value) {
    ServerRole r;
    from_json(json{{"id", "r"}, {"name", "R"}, {"permissions", value}}, r);
    return r;
}
ChannelPermissionOverride override_with(const json& allow, const json& deny) {
    ChannelPermissionOverride o;
    from_json(json{{"allow", allow}, {"deny", deny}}, o);
    return o;
}
} // namespace

TEST(PermissionParsing, NegativeRolePermissionsGrantNothing) {
    EXPECT_EQ(role_with_permissions(-1).permissions, 0u);
    EXPECT_EQ(role_with_permissions(std::numeric_limits<std::int64_t>::min()).permissions, 0u);
}

TEST(PermissionParsing, NonIntegerRolePermissionsGrantNothing) {
    EXPECT_EQ(role_with_permissions(3.0).permissions, 0u);
    EXPECT_EQ(role_with_permissions(true).permissions, 0u);
    EXPECT_EQ(role_with_permissions(nullptr).permissions, 0u);
    EXPECT_EQ(role_with_permissions("0x-1").permissions, 0u);
    EXPECT_EQ(role_with_permissions(json::array()).permissions, 0u);
}

TEST(PermissionParsing, RolePermissionsAreMaskedToDefinedFlags) {
    // Legacy unsigned integers and hex strings both still parse — minus any
    // bit this build does not define.
    EXPECT_EQ(role_with_permissions(std::uint64_t{0x1f}).permissions, 0x1fu);
    EXPECT_EQ(role_with_permissions("0x1f").permissions, 0x1fu);
    EXPECT_EQ(role_with_permissions("ffffffffffffffff").permissions, permission::kAllFlags);
    EXPECT_EQ(role_with_permissions(~std::uint64_t{0}).permissions, permission::kAllFlags);
    EXPECT_EQ(role_with_permissions(std::uint64_t{1} << 63).permissions, 0u);
}

TEST(PermissionParsing, NegativeOverrideValuesAreIgnored) {
    const auto o = override_with(-1, -1);
    EXPECT_EQ(o.allow, 0u);
    EXPECT_EQ(o.deny, 0u);
    const auto f = override_with(1.5, "nope");
    EXPECT_EQ(f.allow, 0u);
    EXPECT_EQ(f.deny, 0u);
}

TEST(PermissionParsing, OverrideValuesAreMaskedToDefinedFlags) {
    const auto o = override_with("ffffffffffffffff", std::uint64_t{0x3});
    EXPECT_EQ(o.allow, permission::kAllFlags);
    EXPECT_EQ(o.deny, 0x3u);
    // And what is written is what is read back.
    json j;
    to_json(j, o);
    ChannelPermissionOverride back;
    from_json(j, back);
    EXPECT_EQ(back.allow, o.allow);
    EXPECT_EQ(back.deny, o.deny);
}

// ---------------------------------------------------------------------------
// Integers in untrusted state (P-3)
// ---------------------------------------------------------------------------
// value(key, int) narrowed a 64-bit number into an int (2^32 became 0) and
// threw type_error on a string, failing the whole event. Now: type-checked,
// clamped, and the default when it is not an integer at all.

TEST(StateIntegers, RolePositionIsClampedAndTypeChecked) {
    auto pos = [](const json& v) {
        ServerRole r;
        from_json(json{{"id", "r"}, {"position", v}}, r);
        return r.position;
    };
    EXPECT_EQ(pos(10), 10);
    EXPECT_EQ(pos(-5), 0);
    EXPECT_EQ(pos(std::int64_t{1} << 32), limits::kMaxRolePosition);
    EXPECT_EQ(pos(~std::uint64_t{0}), limits::kMaxRolePosition);
    EXPECT_EQ(pos("10"), 0);
    EXPECT_EQ(pos(2.5), 0);
}

TEST(StateIntegers, SlowmodeIsClampedAndTypeChecked) {
    auto slow = [](const json& v) {
        ChannelSettingsContent c;
        from_json(json{{"slowmode_seconds", v}}, c);
        return c.slowmode_seconds;
    };
    EXPECT_EQ(slow(30), 30);
    EXPECT_EQ(slow(-1), 0);
    EXPECT_EQ(slow(std::int64_t{1} << 40), limits::kMaxSlowmodeSeconds);
    EXPECT_EQ(slow("30"), 0);
    ChannelSettingsContent absent;
    EXPECT_NO_THROW(from_json(json::object(), absent));
    EXPECT_EQ(absent.slowmode_seconds, 0);
}

TEST(StateIntegers, MaxParticipantsIsClampedAndTypeChecked) {
    auto maxp = [](const json& v) {
        VoiceChannelContent c;
        from_json(json{{"enabled", true}, {"max_participants", v}}, c);
        return c.max_participants;
    };
    EXPECT_EQ(maxp(8), 8);
    EXPECT_EQ(maxp(-3), 0);  // unlimited, as a negative always behaved
    EXPECT_EQ(maxp(std::int64_t{1} << 32), limits::kMaxVoiceParticipants);
    EXPECT_EQ(maxp("8"), 0);
}

TEST(StateIntegers, PowerLevelsAreClampedAndBadEntriesDropped) {
    json j = {
        {"users_default", "high"},             // not an integer: default
        {"ban", std::int64_t{1} << 40},        // clamped
        {"kick", -(std::int64_t{1} << 40)},    // clamped
        {"redact", 75},
        {"users", {{"@a:x", 100}, {"@b:x", "100"}, {"@c:x", 1.5},
                   {"@d:x", std::int64_t{1} << 33}}},
        {"events", {{"m.room.name", 50}, {"m.bad", nullptr}}},
    };
    PowerLevelsContent p;
    ASSERT_NO_THROW(from_json(j, p));
    EXPECT_EQ(p.users_default, 0);
    EXPECT_EQ(p.ban, limits::kMaxPowerLevel);
    EXPECT_EQ(p.kick, limits::kMinPowerLevel);
    EXPECT_EQ(p.redact, 75);
    EXPECT_EQ(p.state_default, 50);
    ASSERT_EQ(p.users.size(), 2u);
    EXPECT_EQ(p.users["@a:x"], 100);
    EXPECT_EQ(p.users["@d:x"], limits::kMaxPowerLevel);
    ASSERT_EQ(p.events.size(), 1u);
    EXPECT_EQ(p.events["m.room.name"], 50);
}

TEST(StateIntegers, CallVersionAsStringIsTheDefault) {
    // Matrix VoIP v1 writes "version": "1". That used to throw.
    CallHangupContent c;
    ASSERT_NO_THROW(from_json(json{{"call_id", "c1"}, {"version", "1"}}, c));
    EXPECT_EQ(c.version, 1);
}

// ---------------------------------------------------------------------------
// rooms.leave
// ---------------------------------------------------------------------------

namespace {
RoomEvent own_leave_event() {
    RoomEvent ev;
    ev.event_id = "$leave:example.com";
    ev.room_id = "!gone:example.com";
    ev.sender = "@mod:example.com";
    ev.type = "m.room.member";
    ev.state_key = "@alice:example.com";
    ev.content.data = {{"membership", "leave"}};
    ev.origin_server_ts = 1700000000000;
    return ev;
}
} // namespace

TEST(SyncResponse, LeaveSectionRoundTrips) {
    SyncResponse sync;
    sync.next_batch = "s9";
    sync.rooms.leave["!gone:example.com"] = SyncLeftRoom{"kick", {own_leave_event()}};
    sync.rooms.leave["!deleted:example.com"] = SyncLeftRoom{"deleted", {}};

    json j;
    to_json(j, sync);
    const auto& leave = j["rooms"]["leave"];
    ASSERT_TRUE(leave.is_object());
    EXPECT_EQ(leave["!gone:example.com"]["bsfchat.reason"], "kick");
    ASSERT_EQ(leave["!gone:example.com"]["timeline"]["events"].size(), 1u);
    EXPECT_EQ(leave["!gone:example.com"]["timeline"]["events"][0]["state_key"],
              "@alice:example.com");
    EXPECT_TRUE(leave["!deleted:example.com"]["timeline"]["events"].is_array());
    EXPECT_TRUE(leave["!deleted:example.com"]["timeline"]["events"].empty());

    SyncResponse parsed;
    from_json(j, parsed);
    ASSERT_EQ(parsed.rooms.leave.size(), 2u);
    EXPECT_EQ(parsed.rooms.leave["!gone:example.com"].reason, "kick");
    ASSERT_EQ(parsed.rooms.leave["!gone:example.com"].timeline_events.size(), 1u);
    EXPECT_EQ(parsed.rooms.leave["!gone:example.com"].timeline_events[0].event_id,
              "$leave:example.com");
    EXPECT_EQ(parsed.rooms.leave["!deleted:example.com"].reason, "deleted");
}

TEST(SyncResponse, NoLeavesWritesNoLeaveSection) {
    // Byte-compatible with every response before the field existed — and it
    // is what keeps the server's own post-processing (which writes
    // rooms.leave into the JSON after to_json) from meeting a second copy.
    SyncResponse sync;
    sync.next_batch = "s1";
    sync.rooms.join["!room:example.com"] = JoinedRoom{};
    json j;
    to_json(j, sync);
    EXPECT_FALSE(j["rooms"].contains("leave"));

    // What that post-processing produces still parses into the field.
    j["rooms"]["leave"] = {{"!x:example.com",
                            {{"bsfchat.reason", "ban"}, {"timeline", {{"events", json::array()}}}}}};
    SyncResponse parsed;
    from_json(j, parsed);
    ASSERT_EQ(parsed.rooms.leave.count("!x:example.com"), 1u);
    EXPECT_EQ(parsed.rooms.leave["!x:example.com"].reason, "ban");
}

TEST(SyncResponse, MalformedLeaveSectionIsTolerated) {
    // Not an object at all: empty, and the rest of the sync still parses.
    for (const json& bad : {json("nope"), json::array({1, 2}), json(nullptr), json(7)}) {
        json j = {{"next_batch", "s1"}, {"rooms", {{"join", json::object()}, {"leave", bad}}}};
        SyncResponse parsed;
        ASSERT_NO_THROW(from_json(j, parsed));
        EXPECT_TRUE(parsed.rooms.leave.empty());
        EXPECT_EQ(parsed.next_batch, "s1");
    }

    // Entries: the key is the instruction, so a garbled value keeps the room
    // id and loses only what it garbled.
    json good_event;
    to_json(good_event, own_leave_event());
    json j = {
        {"next_batch", "s2"},
        {"rooms", {{"leave", {
            {"!a:x", "not an object"},
            {"!b:x", {{"bsfchat.reason", 42}, {"timeline", "nope"}}},
            {"!c:x", {{"bsfchat.reason", "leave"},
                      {"timeline", {{"events", json::array({
                          json{{"type", "m.room.member"}},   // no sender: skipped
                          json("garbage"),                   // not an object: skipped
                          good_event})}}}}},
            {"", {{"bsfchat.reason", "leave"}}},             // no room id: skipped
        }}}},
    };
    SyncResponse parsed;
    ASSERT_NO_THROW(from_json(j, parsed));
    ASSERT_EQ(parsed.rooms.leave.size(), 3u);
    EXPECT_TRUE(parsed.rooms.leave["!a:x"].reason.empty());
    EXPECT_TRUE(parsed.rooms.leave["!b:x"].reason.empty());
    EXPECT_TRUE(parsed.rooms.leave["!b:x"].timeline_events.empty());
    EXPECT_EQ(parsed.rooms.leave["!c:x"].reason, "leave");
    ASSERT_EQ(parsed.rooms.leave["!c:x"].timeline_events.size(), 1u);
    EXPECT_EQ(parsed.rooms.leave["!c:x"].timeline_events[0].sender, "@mod:example.com");
}
