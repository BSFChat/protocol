// The refusal-reason wire contract.
//
// `bsfchat.errcode` and its BSFCHAT.* values are things a CLIENT keys on, in a
// different repository, compiled separately. Nothing links the two sides
// together, so a typo here is not a build error anywhere — it is a code that
// silently never matches, which is strictly worse than having no code at all:
// the client falls back to the server's prose and looks like it is working,
// while the branch that was supposed to fire never does.
//
// So every string is pinned as a LITERAL below, written out by hand rather
// than referenced through the constant it is checking. A test that says
//   EXPECT_EQ(refusal::kInviteNotInRoom, refusal::kInviteNotInRoom)
// passes against any typo, which is the mistake this file exists to not make.
//
// The other half is the envelope. `errcode` must stay M_FORBIDDEN (see the
// long comment in ErrorCodes.h: the client's generic 403 routing keys on it),
// the sentence must stay in `error`, and a refusal with no reason must
// serialise to exactly the object it did before this field existed.

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "bsfchat/ErrorCodes.h"

using namespace bsfchat;

namespace {

// Written out by hand. Do not replace with the constants.
TEST(RefusalReasons, EveryCodeIsSpeltExactlyThis) {
    EXPECT_EQ(refusal::kField, "bsfchat.errcode");

    EXPECT_EQ(refusal::kInviteNotInRoom, "BSFCHAT.INVITE_NOT_IN_ROOM");
    EXPECT_EQ(refusal::kInviteDirectRoom, "BSFCHAT.INVITE_DIRECT_ROOM");
    EXPECT_EQ(refusal::kInviteNoPermission, "BSFCHAT.INVITE_NO_PERMISSION");
    EXPECT_EQ(refusal::kInviteTargetBannedRoom, "BSFCHAT.INVITE_TARGET_BANNED_ROOM");
    EXPECT_EQ(refusal::kInviteTargetBannedServer, "BSFCHAT.INVITE_TARGET_BANNED_SERVER");
    EXPECT_EQ(refusal::kInviteNoSuchAccount, "BSFCHAT.INVITE_NO_SUCH_ACCOUNT");
    EXPECT_EQ(refusal::kInviteTargetDeactivated, "BSFCHAT.INVITE_TARGET_DEACTIVATED");
}

// Two reasons that compare equal are two refusals a client cannot tell apart,
// which is the entire defect this work removes. Cheap to check, and it catches
// the copy-paste that a by-hand list invites.
TEST(RefusalReasons, NoTwoCodesCollide) {
    const std::vector<std::string_view> all = {
        refusal::kInviteNotInRoom,
        refusal::kInviteDirectRoom,
        refusal::kInviteNoPermission,
        refusal::kInviteTargetBannedRoom,
        refusal::kInviteTargetBannedServer,
        refusal::kInviteNoSuchAccount,
        refusal::kInviteTargetDeactivated,
    };
    std::set<std::string_view> unique(all.begin(), all.end());
    EXPECT_EQ(unique.size(), all.size());

    // And none of them is a PREFIX of another. Nothing is supposed to match a
    // reason by prefix — that is the substring mistake being retired — but a
    // code that contains another leaves the old failure mode available to
    // whoever writes the next consumer in a hurry. M_UNKNOWN / M_UNKNOWN_TOKEN
    // is the standing example of how that goes (client/src/net/AuthError.h).
    for (const auto& a : all) {
        for (const auto& b : all) {
            if (a == b) continue;
            EXPECT_FALSE(b.starts_with(a)) << b << " starts with " << a;
        }
    }
}

// The namespace rule, checked rather than assumed: a code that lost its prefix
// could collide with a spec errcode the day Matrix defines one.
TEST(RefusalReasons, EveryCodeIsNamespaced) {
    for (std::string_view code : {refusal::kInviteNotInRoom,
                                  refusal::kInviteDirectRoom,
                                  refusal::kInviteNoPermission,
                                  refusal::kInviteTargetBannedRoom,
                                  refusal::kInviteTargetBannedServer,
                                  refusal::kInviteNoSuchAccount,
                                  refusal::kInviteTargetDeactivated}) {
        EXPECT_TRUE(code.starts_with("BSFCHAT.")) << code;
        EXPECT_FALSE(code.starts_with("M_")) << code;
    }
}

// ── the envelope ─────────────────────────────────────────────────────────

TEST(RefusalReasons, TheErrcodeStaysMForbidden) {
    // The whole compatibility story in one assertion. A client that routes a
    // 403 on errcode == "M_FORBIDDEN" — which the shipped one does — must keep
    // routing it after the server starts classifying its refusals.
    const auto e = MatrixError::forbidden("User is banned from this room",
                                          refusal::kInviteTargetBannedRoom);
    const auto j = e.to_json();
    EXPECT_EQ(j.at("errcode").get<std::string>(), "M_FORBIDDEN");
}

TEST(RefusalReasons, TheSentenceIsStillThereAndIsNotTheReason) {
    // The reason is for the client's LOGIC. It does not replace the sentence,
    // because an unrecognised reason has to degrade to showing the server's
    // own words rather than to showing a machine code to a person.
    const auto j = MatrixError::forbidden("User is banned from this room",
                                          refusal::kInviteTargetBannedRoom)
                       .to_json();
    EXPECT_EQ(j.at("error").get<std::string>(), "User is banned from this room");
    EXPECT_EQ(j.at("bsfchat.errcode").get<std::string>(),
              "BSFCHAT.INVITE_TARGET_BANNED_ROOM");
}

TEST(RefusalReasons, ARefusalWithNoReasonIsTheExactBodyItAlwaysWas) {
    // Every refusal that has not been classified must be byte-identical to
    // what it was before this field existed — including the ones that are
    // byte-identical to EACH OTHER on purpose (PermissionsHandler's kRefusal).
    // An empty reason written out as "" would break that silently.
    const auto j = MatrixError::forbidden("Forbidden").to_json();
    EXPECT_FALSE(j.contains("bsfchat.errcode"));
    EXPECT_EQ(j.dump(), R"({"errcode":"M_FORBIDDEN","error":"Forbidden"})");
}

TEST(RefusalReasons, TwoRefusalsThatShareASentenceAndAReasonStayIndistinguishable) {
    // The oracle rule, as a property rather than as a comment: equal inputs
    // must give an equal body, so classifying a refusal can never be the thing
    // that tells two deliberately identical answers apart.
    const auto a = MatrixError::forbidden("Insufficient permissions to invite",
                                          refusal::kInviteNoPermission)
                       .to_json()
                       .dump();
    const auto b = MatrixError::forbidden("Insufficient permissions to invite",
                                          refusal::kInviteNoPermission)
                       .to_json()
                       .dump();
    EXPECT_EQ(a, b);
}

TEST(RefusalReasons, ItSurvivesARoundTrip) {
    const auto original = MatrixError::forbidden("There is no account on this server with that id",
                                                 refusal::kInviteNoSuchAccount);
    const auto parsed = MatrixError::from_json(original.to_json());
    EXPECT_EQ(parsed.errcode, original.errcode);
    EXPECT_EQ(parsed.error, original.error);
    EXPECT_EQ(parsed.reason, original.reason);
}

TEST(RefusalReasons, AnOlderServersBodyParsesWithAnEmptyReason) {
    // What a client compiled against this header sees when it talks to a
    // server that predates the field. Empty, not a throw — "this server does
    // not classify" is a normal answer and means "use the sentence".
    const auto parsed = MatrixError::from_json(
        nlohmann::json::parse(R"({"errcode":"M_FORBIDDEN","error":"Not a member of this room"})"));
    EXPECT_EQ(parsed.errcode, "M_FORBIDDEN");
    EXPECT_EQ(parsed.error, "Not a member of this room");
    EXPECT_TRUE(parsed.reason.empty());
}

} // namespace
