#include <gtest/gtest.h>
#include "bsfchat/MatrixTypes.h"

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
