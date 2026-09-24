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
    // One member's EFFECTIVE SERVER-SCOPE permission mask, computed by the
    // server. GET /permissions/{userId}.
    //
    // bsfchat.* namespaced for the same reason everything above it is, and it is
    // a READ-ONLY sibling of kRoles rather than a verb on it because it answers a
    // different question. kRoles is the role DOCUMENT — what roles exist and what
    // each one grants. This is the answer after that document has been applied to
    // a particular member's assignments and the ADMINISTRATOR short-circuit has
    // run. A caller that has the document still cannot answer it: it also needs
    // that member's assignment, which /sync only ever carried as a best-effort
    // mirror into ONE room (see RoleBootstrap::pick_server_state_mirror_room), so
    // an integration invited into any other channel had no way to find out.
    //
    // Exists so that an integration asking "may this person administer me?" can
    // ask the server rather than keep an allowlist in its own config. The
    // alternative every bot author reaches for otherwise is to reimplement the
    // OR-roles-then-short-circuit algorithm against mirrored state, which is the
    // client's PermissionMath and has drifted from this repo before.
    //
    // SERVER scope only, and deliberately: there is no channel variant. The flags
    // an integration needs to gate on — MANAGE_BOTS, MANAGE_SERVER, ADMINISTRATOR
    // — are all evaluated at server scope by the endpoints that enforce them, and
    // a per-channel answer would let this endpoint disagree with the thing it is
    // supposed to predict. It would also make the endpoint report on the
    // existence and override shape of channels, which is a disclosure this read
    // has no reason to make.
    constexpr std::string_view kPermissions = "/_matrix/client/v3/bsfchat/permissions";

    // Account linking: attaching an OIDC identity to the account the caller is
    // already signed in as, so one human is one account.
    //
    // THE PROBLEM THESE EXIST FOR. An m.login.token sign-in derives a user id
    // from the identity provider's `sub` claim ("@oidc_<sub>:server") and
    // creates that account if it is missing. Nothing relates it to the local
    // account the same person already had, so a server owner who registered
    // with a password and later signed in through the IdP ends up owning two
    // accounts with no connection between them — and on production, three.
    // Roles, DMs and history do not follow the person, and the client cannot
    // even tell the reader that the two names are one human.
    //
    // bsfchat.* namespaced: Matrix's own account-linking vocabulary is 3PIDs
    // (email/MSISDN via an identity server), which is a different mechanism
    // answering a different question, and squatting /account/3pid with
    // something that is not a 3PID would break any spec-conformant client.
    //
    // POST link_identity is the write. It is authenticated TWICE over, and
    // that is the security property: the bearer token proves control of the
    // surviving account, and the OIDC id_token in the body proves control of
    // the identity being attached. Neither alone is sufficient, so asserting
    // an identity can never claim somebody else's account.
    constexpr std::string_view kLinkIdentity =
        "/_matrix/client/v3/bsfchat/account/link_identity";
    // GET — the identities linked to the CALLER's own account, and nobody
    // else's. There is no lookup by user id: "which identity provider account
    // is @josh?" is not a question one member gets to ask about another.
    constexpr std::string_view kLinkedIdentities =
        "/_matrix/client/v3/bsfchat/account/linked_identities";

    // The channel directory: every channel on this server that the CALLER is
    // allowed to see, member or not.
    //
    // bsfchat.* namespaced for the same reason the four above are, and
    // pointedly NOT Matrix's /publicRooms. That endpoint answers "which rooms
    // has this server published in its room directory", a flag on the room;
    // this one answers "which channels may THIS caller be told about", which is
    // a per-caller VIEW_CHANNEL evaluation. Serving the latter from the former
    // path would hand a spec-shaped name to a non-spec answer, and the first
    // client to trust the name would enumerate every private channel on the
    // server — `visibility` is "public" on every channel here, including the
    // private ones (docs/membership-vs-visibility.md).
    constexpr std::string_view kChannelDirectory = "/_matrix/client/v3/bsfchat/channels";

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
    // ROOM account data, not a room event: how far this reader has read in
    // this room. Matrix's own type, so a conventional client finds its read
    // marker where it expects to. See `fully_read` below for the content.
    constexpr std::string_view kFullyRead = "m.fully_read";
    // Global account data: the reader's block list. Named here because /sync
    // now carries account data and the two ends have to agree on the string;
    // the server's own copy is server/src/store/SqliteStore.h.
    constexpr std::string_view kIgnoredUserList = "m.ignored_user_list";
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

// The content of an `m.fully_read` room account-data document.
//
// `event_id` is the spec's whole story: the marker names an event, and a
// client is expected to already know where that event sits.
//
// THIS CLIENT DOES NOT. Its unread dot is arithmetic on origin_server_ts
// (client/src/core/ReadState.h) and the event the marker names is very often
// one it has never loaded — a phone read the room, the desktop has a sidebar
// row for it and no timeline. Resolving the id would mean a /messages request
// per room per marker, on the one endpoint a client polls continuously, to
// recover a number the server already had in its hand when it wrote the row.
//
// So the timestamp travels beside the id, under a bsfchat.* key because it is
// not the spec's. Both are written; a client that knows only the spec reads
// `event_id` and ignores the rest, and this one reads the timestamp and never
// has to ask.
namespace fully_read {
    constexpr std::string_view kEventId = "event_id";
    constexpr std::string_view kOriginServerTs = "bsfchat.origin_server_ts";
} // namespace fully_read

// The `type` field of a `bsfchat.room.type` state event, and the same strings
// as they appear in the channel directory's `type`.
//
// Promoted here from the three string literals the server spelled by hand
// (RoomHandler's create path, RoomVisibility's category predicate, SyncEngine's
// stub rule) because the directory puts them ON THE WIRE as a contract a bot
// author switches on. A wire value with no name is a wire value that gets
// misspelled on one of the two sides.
//
// The existing server call sites still spell their literals; they are not
// rewritten here so this change stays a pure addition. That is a follow-up, not
// a difference of opinion about where the constant lives.
namespace room_type {
    constexpr std::string_view kText = "text";
    constexpr std::string_view kVoice = "voice";
    // A sidebar container, not a channel: it holds no messages and its children
    // name it as their parent.
    constexpr std::string_view kCategory = "category";
} // namespace room_type

// @mentions. The block on the wire is MSC3952's:
//
//     "m.mentions": {
//         "user_ids": ["@alice:host", ...],
//         "room": true,
//         "bsfchat.role_ids": ["mod", ...]     // ours
//     }
//
// Role mentions have no MSC, so the key is vendor-prefixed and lives INSIDE
// `m.mentions` rather than beside it. Both halves of that are deliberate:
// a spec-compliant third-party client ignores an unknown key in the block and
// is unaffected, and keeping every mention in one object means the rules that
// already govern the block — the server's "never recorded for an edit" rule,
// and the client's matching notifiedMentions() rule — cover role mentions
// without a second code path that could drift out of step with the first.
namespace mention {
    // Key for the role list inside the `m.mentions` object.
    constexpr std::string_view kRoleIdsKey = "bsfchat.role_ids";

    // Storage sentinels for event_mentions.user_id. A mention of N members is
    // ONE row carrying a sentinel, never N rows — see SqliteStore's mention
    // section for why the fan-out lives in the read and not the write.
    //
    // Both are unspellable as Matrix user ids, which is what stops a client
    // claiming a broadcast by naming one in `m.mentions.user_ids`: "@room" has
    // no ':' so UserId::parse rejects it, and a role sentinel is "@role/" plus
    // a role id, which likewise has no ':' (role ids are validated against the
    // roles list, and one containing ':' is refused). The send path rejects
    // both spellings explicitly as well; this is the belt to that's braces.
    constexpr std::string_view kRoomSentinel = "@room";
    constexpr std::string_view kRolePrefix = "@role/";
} // namespace mention

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

// The key a SUCCESS response carries when the request succeeded and will not
// do what the caller expects it to.
//
// There is exactly one situation on this server that needs such a thing, and
// it is the one that produced this constant: a bot holding no roles joins a
// public channel and the join genuinely succeeds, because membership is not a
// permission here — and then every read and every send in that room is a 403.
// Nothing in between says so. `POST /bots` reported a bot, `POST /token`
// reported a token, `POST /join` reported a join, and the first thing to
// mention the actual state of affairs was a refusal that named the channel.
//
// A WARNING ON A 200, rather than turning the join into a refusal, because
// the join is not wrong: bot scoping deliberately lets a bot into a public
// room and gives it nothing there (docs/bot-scoping.md §4). What was wrong was
// that the sequence reported success four times and meant it three.
//
// Advisory and free-form — a sentence for a person, never a value to branch
// on. A client with something to say about the condition should compute it
// from `GET /bsfchat/bots/{id}/access`, which answers it properly. An unknown
// key on a Matrix response is ignored, so nothing that has not heard of this
// is affected.
constexpr std::string_view kWarningKey = "bsfchat.warning";

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

    // ── How big a single message may be ───────────────────────────────────
    //
    // There was no bound of any kind, client-side or server-side, on the text
    // of an m.room.message. `kMaxMessagesLimit` above is a PAGE SIZE and was
    // repeatedly mistaken for a size limit; it bounds how many events come
    // back from /messages and says nothing about how big one of them is.
    //
    // Unbounded, one authenticated member could post a multi-megabyte body,
    // and every other member then paid for it: it is stored, indexed by FTS
    // (which roughly doubles it), pushed through /sync to every member of the
    // room, and returned by every backfill of that page forever after. Sending
    // costs one request; receiving costs N. That is an amplifier, and it is
    // cheaper to send than to receive.
    //
    // BYTES, not characters. Three reasons: bytes are what is stored, indexed
    // and put on the wire, so bytes are what the limit is actually defending;
    // `std::string::size()` needs no UTF-8 decode on the hot write path; and a
    // codepoint count still has to be paired with a byte ceiling anyway,
    // because 2,000 four-byte codepoints are 8 KB. The unfairness a byte limit
    // normally carries — non-Latin text gets fewer characters for the same
    // budget — is answered by setting it high rather than by counting
    // codepoints: at 16 KiB, worst-case 4-bytes-per-character text still gets
    // ~4,000 characters, which is twice what Discord allows anybody.
    //
    // 16 KiB is deliberately far above a typed message and comfortably above a
    // deliberate paste (a stack trace, a config block, ~250 lines of code).
    // This is not a style rule about message length — it is the point past
    // which one member's send becomes everyone else's bandwidth.
    constexpr size_t kMaxMessageBodyBytes = 16 * 1024;

    // `formatted_body` is the SAME message as `body` after markup, and it is
    // legitimately several times larger: a mention that is "@alice" in the
    // body is an <a href="https://matrix.to/#/@alice:host">…</a> in the HTML,
    // so a formatted roster of fifty names is ~1 KB of body against ~6 KB of
    // HTML. Holding it to the plain-text ceiling would reject messages whose
    // visible text is well inside the limit, and the composer — which can only
    // count what the user typed — could not warn about it in advance.
    //
    // So: its own ceiling, derived rather than separately configurable, at 4x
    // the body. One number for an operator to set, and the derivation keeps
    // the two from drifting apart. The multiplier is the observed worst case
    // for mention-dense HTML with headroom, not a guess at "some HTML".
    constexpr size_t kFormattedBodyMultiplier = 4;
    constexpr size_t kMaxFormattedBodyBytes =
        kMaxMessageBodyBytes * kFormattedBodyMultiplier;

    // Ceiling on the whole JSON body of one PUT /rooms/{id}/send/{type}/{txn},
    // whatever the type.
    //
    // The text ceilings above cover m.room.message, which is the type this was
    // found on, but the send allowlist also admits m.reaction and the
    // call-signalling types, and `content` on any of them is free-form JSON
    // that is stored and delivered exactly the same way. Bounding only the
    // fields we happen to read would leave the hole open under a different
    // key: an m.reaction whose `key` is a valid 8 bytes and which carries a
    // 4 MB field nobody parses is the same amplifier with a different name.
    //
    // So the outer bound is on the REQUEST, not on any field, and it is
    // checked before the JSON is parsed — which also means a hostile payload
    // never reaches the parser. The call types keep a generous allowance on
    // purpose: an SDP offer with a long candidate list is a real several-KB
    // document and refusing one breaks a call, which is a worse failure than
    // the one being prevented.
    //
    // 128 KiB leaves room for a maximal message — 16 KiB body + 64 KiB
    // formatted_body + fifty mention ids + relation blocks — with slack, so
    // the field limits are what a legitimate oversize message trips, and this
    // is what a payload with no legitimate shape at all trips.
    constexpr size_t kMaxEventContentBytes = 128 * 1024;

    // The outer bound has to leave room for a message that passes both field
    // bounds, or the field bounds become unreachable and their error messages
    // become lies — the caller would be told "body may be 16 KiB" by a server
    // that refuses the request before it ever looks at `body`. Asserted rather
    // than commented because the three numbers are edited independently.
    // Config::validate() enforces the same relation on the operator-set value.
    static_assert(kMaxEventContentBytes >
                      kMaxMessageBodyBytes + kMaxFormattedBodyBytes,
                  "kMaxEventContentBytes must admit a maximal body + "
                  "formatted_body, with room for the rest of the event");

    // Ceiling on `m.mentions.user_ids` entries in a single event. A mention is
    // a write per target plus a push-queue row per target's pusher, so an
    // unbounded list is a cheap amplification primitive. Well above any real
    // message; a client hitting this is broken or hostile.
    constexpr size_t kMaxMentionsPerEvent = 50;
    // Ceiling on `m.mentions.bsfchat.role_ids` entries in a single event. Far
    // tighter than the user ceiling and for a different reason: a role mention
    // costs ONE stored row however many members hold the role, so the write is
    // not the amplification risk — the read is. Every recorded role sentinel
    // widens the `user_id IN (...)` list that /sync's unread-mention query runs
    // per member, so the cost of a silly list is paid by everyone else's sync,
    // forever. No real message names more than a couple of roles.
    constexpr size_t kMaxRoleMentionsPerEvent = 8;
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
