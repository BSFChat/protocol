#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace bsfchat {

// Matrix error codes
namespace error_code {
    constexpr std::string_view kForbidden = "M_FORBIDDEN";
    constexpr std::string_view kUnknownToken = "M_UNKNOWN_TOKEN";
    constexpr std::string_view kMissingToken = "M_MISSING_TOKEN";
    constexpr std::string_view kBadJson = "M_BAD_JSON";
    constexpr std::string_view kNotJson = "M_NOT_JSON";
    constexpr std::string_view kNotFound = "M_NOT_FOUND";
    constexpr std::string_view kUserInUse = "M_USER_IN_USE";
    constexpr std::string_view kInvalidUsername = "M_INVALID_USERNAME";
    constexpr std::string_view kRoomInUse = "M_ROOM_IN_USE";
    constexpr std::string_view kUnknown = "M_UNKNOWN";
    constexpr std::string_view kLimitExceeded = "M_LIMIT_EXCEEDED";
    constexpr std::string_view kExclusive = "M_EXCLUSIVE";
    constexpr std::string_view kGuestAccessForbidden = "M_GUEST_ACCESS_FORBIDDEN";
    constexpr std::string_view kInvalidParam = "M_INVALID_PARAM";
    constexpr std::string_view kTooLarge = "M_TOO_LARGE";
    constexpr std::string_view kUnrecognized = "M_UNRECOGNIZED";
} // namespace error_code

// ── Refusal reasons ──────────────────────────────────────────────────────
//
// A second, FINER code that rides alongside `errcode` on a refusal, so a
// client can act on WHY it was refused instead of reading the prose.
//
// ── The problem ──────────────────────────────────────────────────────────
//
// M_FORBIDDEN is the only errcode Matrix gives an endpoint for "no", and this
// server says no for many distinct reasons that call for entirely different
// advice. POST /rooms/{id}/invite alone has seven. Until this existed the
// `error` sentence was the whole of the signal, so the client matched
// SUBSTRINGS of it (client/src/model/ChannelInviteModel.cpp, explainFailure)
// — brittle in both directions: reword a message here and the client silently
// mis-attributes or falls through, add a refusal and an earlier fragment may
// swallow it.
//
// ── Why this is not just put in `errcode` ────────────────────────────────
//
// Because `errcode` is load-bearing for callers that have never heard of
// these. The client's own generic error handling keys on it — a 403 body is
// routed by `code == "M_FORBIDDEN"` in ServerConnection.cpp, and AuthError /
// SyncBackoff key on M_UNKNOWN_TOKEN — and so does anything Matrix-generic
// pointed at this server. Replacing M_FORBIDDEN with BSFCHAT.SOMETHING would
// break every one of them on the day the server upgraded, which is precisely
// the compatibility that the substring matching, for all its faults, has.
//
// So `errcode` keeps saying M_FORBIDDEN and this is an EXTRA field. That is
// not a second error envelope: the Matrix error object is open, MatrixError
// already carries exactly such an optional extra in `retry_after_ms`, and an
// unknown key is ignored by every client that does not look for it.
//
// ── Naming ───────────────────────────────────────────────────────────────
//
// The KEY is `bsfchat.errcode`, lowercase-namespaced like every other
// bsfchat-owned JSON key in Constants.h (`bsfchat.role_ids`, `bsfchat.bot`)
// and like Matrix's own advice that custom fields carry a namespace so they
// cannot collide with something the spec later defines.
//
// The VALUES are `BSFCHAT.<SCREAMING_SNAKE>`, because they are errcodes and
// Matrix errcodes are uppercase; the spec's rule for a custom one is that it
// is namespaced, which this is. The namespace word is `BSFCHAT` rather than
// `COM.BSFCHAT` to stay with the bare `bsfchat` this project already uses
// everywhere else rather than introducing a second spelling of ourselves.
//
// ── Two standing rules ───────────────────────────────────────────────────
//
// 1. A REASON NEVER REPLACES THE SENTENCE. `error` stays a sentence a person
//    can read, and stays as it was — an older client matching substrings must
//    keep working against a newer server, and a client that does not
//    recognise a reason must fall back to showing the server's own words
//    rather than guessing.
//
// 2. A REASON MUST NOT DISCLOSE MORE THAN THE SENTENCE ALREADY DID. Where two
//    refusals are deliberately byte-identical because telling them apart
//    would leak — PermissionsHandler's kRefusal, which answers "stranger",
//    "invisible member" and "no such account" with one string — they must
//    also carry the SAME reason, or this becomes a cheaper oracle than the
//    prose was. Adding a reason is only free where the prose already
//    distinguished the cases.
namespace refusal {
    // The JSON key. Server writes it; client reads it.
    constexpr std::string_view kField = "bsfchat.errcode";

    // ── POST /rooms/{roomId}/invite ──────────────────────────────────────
    //
    // All seven of handle_invite's refusals, in the order the handler
    // evaluates them. That order is itself a boundary (see kNoSuchAccount in
    // RoomHandler.cpp): the three target-shaped reasons below the permission
    // check are unreachable without MANAGE_CHANNELS, which is what keeps the
    // endpoint from being an account-existence oracle for an ordinary member.
    // A caller who fails the permission check gets kInviteNoPermission for a
    // real id and a fictional one alike.

    // The CALLER is not in the room they are inviting into.
    constexpr std::string_view kInviteNotInRoom = "BSFCHAT.INVITE_NOT_IN_ROOM";
    // The room is a DM. Not widenable by anyone, including its two members.
    constexpr std::string_view kInviteDirectRoom = "BSFCHAT.INVITE_DIRECT_ROOM";
    // The caller lacks MANAGE_CHANNELS in this room.
    constexpr std::string_view kInviteNoPermission = "BSFCHAT.INVITE_NO_PERMISSION";
    // The TARGET is banned from this room.
    constexpr std::string_view kInviteTargetBannedRoom = "BSFCHAT.INVITE_TARGET_BANNED_ROOM";
    // The TARGET is on the server-wide ban list. Distinct from the above
    // because the remedy is: this one covers every channel.
    constexpr std::string_view kInviteTargetBannedServer = "BSFCHAT.INVITE_TARGET_BANNED_SERVER";
    // No account holds the invited id — a mistyped localpart, an id on
    // another homeserver, or a string that is not an mxid. ONE reason for all
    // three, exactly as there is one sentence for all three: the server
    // cannot be asked which it meant, and three reasons would classify the
    // namespace for whoever wanted that.
    constexpr std::string_view kInviteNoSuchAccount = "BSFCHAT.INVITE_NO_SUCH_ACCOUNT";
    // The TARGET is a deactivated bot. Deactivation is permanent.
    constexpr std::string_view kInviteTargetDeactivated = "BSFCHAT.INVITE_TARGET_DEACTIVATED";
} // namespace refusal

struct MatrixError {
    std::string errcode;
    std::string error;
    // The refusal::k* value, when the refusal has one. Serialised as the
    // `bsfchat.errcode` field and omitted entirely when empty, so every
    // response that does not set one is byte-identical to what it was before
    // this field existed.
    std::string reason;

    [[nodiscard]] nlohmann::json to_json() const;
    static MatrixError from_json(const nlohmann::json& j);

    // `reason` is optional: a refusal nobody has a client-side use for yet
    // carries none, and is exactly the body it was before.
    static MatrixError forbidden(const std::string& msg = "Forbidden",
                                 std::string_view reason = {});
    static MatrixError unknown_token(const std::string& msg = "Invalid or expired token");
    static MatrixError missing_token(const std::string& msg = "Missing access token");
    static MatrixError bad_json(const std::string& msg = "Invalid JSON");
    static MatrixError not_found(const std::string& msg = "Not found");
    static MatrixError user_in_use(const std::string& msg = "Username already taken");
    static MatrixError invalid_username(const std::string& msg = "Invalid username");
    static MatrixError unknown(const std::string& msg = "An unknown error occurred");
    static MatrixError invalid_param(const std::string& msg = "Invalid parameter");
    static MatrixError too_large(const std::string& msg = "Content too large");
    static MatrixError unrecognized(const std::string& msg = "Unrecognized request");
    static MatrixError limit_exceeded(const std::string& msg = "Rate limit exceeded", int retry_after_ms = 0);

    // Optional retry_after_ms field, emitted when set.
    int retry_after_ms = 0;
};

} // namespace bsfchat
