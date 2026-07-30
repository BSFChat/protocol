#include <gtest/gtest.h>
#include "bsfchat/JwtUtils.h"

#include <stdexcept>
#include <string>

using namespace bsfchat;

class JwtTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto [priv, pub] = generate_rsa_keypair();
        private_key = priv;
        public_key = pub;
    }

    std::string private_key;
    std::string public_key;
};

TEST_F(JwtTest, GenerateKeyPair) {
    EXPECT_TRUE(private_key.starts_with("-----BEGIN PRIVATE KEY-----"));
    EXPECT_TRUE(public_key.starts_with("-----BEGIN PUBLIC KEY-----"));
}

TEST_F(JwtTest, SignAndVerify) {
    JwtClaims claims;
    claims.sub = "user-123";
    claims.iss = "https://id.bsfchat.example.com";
    claims.aud = "test-client";
    claims.iat = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    claims.exp = claims.iat + 3600;
    claims.name = "Alice";
    claims.email = "alice@example.com";

    auto token = jwt_sign(claims, private_key, "key-1");
    EXPECT_FALSE(token.empty());

    auto verified = jwt_verify(token, public_key, "https://id.bsfchat.example.com");
    ASSERT_TRUE(verified.has_value());
    EXPECT_EQ(verified->sub, "user-123");
    EXPECT_EQ(verified->iss, "https://id.bsfchat.example.com");
    EXPECT_EQ(verified->name, "Alice");
    EXPECT_EQ(verified->email, "alice@example.com");
}

TEST_F(JwtTest, VerifyWrongIssuer) {
    JwtClaims claims;
    claims.sub = "user-123";
    claims.iss = "https://id.bsfchat.example.com";
    claims.aud = "test-client";
    claims.iat = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    claims.exp = claims.iat + 3600;

    auto token = jwt_sign(claims, private_key, "key-1");
    auto verified = jwt_verify(token, public_key, "https://wrong-issuer.com");
    EXPECT_FALSE(verified.has_value());
}

TEST_F(JwtTest, VerifyExpiredToken) {
    JwtClaims claims;
    claims.sub = "user-123";
    claims.iss = "https://id.bsfchat.example.com";
    claims.aud = "test-client";
    claims.iat = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - 7200;
    claims.exp = claims.iat + 3600; // expired an hour ago

    auto token = jwt_sign(claims, private_key, "key-1");
    auto verified = jwt_verify(token, public_key, "https://id.bsfchat.example.com");
    EXPECT_FALSE(verified.has_value());
}

TEST_F(JwtTest, VerifyWrongKey) {
    JwtClaims claims;
    claims.sub = "user-123";
    claims.iss = "https://id.bsfchat.example.com";
    claims.aud = "test-client";
    claims.iat = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    claims.exp = claims.iat + 3600;

    auto token = jwt_sign(claims, private_key, "key-1");

    // Generate a different key pair
    auto [other_priv, other_pub] = generate_rsa_keypair();
    auto verified = jwt_verify(token, other_pub, "https://id.bsfchat.example.com");
    EXPECT_FALSE(verified.has_value());
}

TEST_F(JwtTest, Base64UrlDecode) {
    // "Hello" in base64url is "SGVsbG8"
    auto decoded = base64url_decode("SGVsbG8");
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, "Hello");

    // Empty input
    auto empty = base64url_decode("");
    EXPECT_TRUE(empty.empty());

    // Test with characters that differ between base64 and base64url
    // base64url uses - and _ instead of + and /
    auto decoded2 = base64url_decode("PDw_Pz4-");
    std::string result2(decoded2.begin(), decoded2.end());
    EXPECT_EQ(result2, "<<??>>");
}

TEST_F(JwtTest, JwkToPemRoundTrip) {
    // Convert public key to JWK, then back to PEM
    auto jwk = pem_to_jwk(public_key, "roundtrip-key");
    auto recovered_pem = jwk_to_pem(jwk);

    // Verify the recovered PEM works: sign with original private key, verify with recovered public key
    JwtClaims claims;
    claims.sub = "roundtrip-user";
    claims.iss = "https://test.example.com";
    claims.aud = "test-client";
    claims.iat = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    claims.exp = claims.iat + 3600;
    claims.name = "Round Trip";

    auto token = jwt_sign(claims, private_key, "roundtrip-key");
    ASSERT_FALSE(token.empty());

    auto verified = jwt_verify(token, recovered_pem, "https://test.example.com");
    ASSERT_TRUE(verified.has_value());
    EXPECT_EQ(verified->sub, "roundtrip-user");
    EXPECT_EQ(verified->name, "Round Trip");
}

TEST_F(JwtTest, PemToJwk) {
    auto jwk = pem_to_jwk(public_key, "key-1");
    EXPECT_EQ(jwk["kty"], "RSA");
    EXPECT_EQ(jwk["alg"], "RS256");
    EXPECT_EQ(jwk["kid"], "key-1");
    EXPECT_TRUE(jwk.contains("n"));
    EXPECT_TRUE(jwk.contains("e"));
}

// ---------------------------------------------------------------------------
// LiveKit access tokens (HS256 + nested `video` grant)
// ---------------------------------------------------------------------------
//
// These decode the token by hand rather than through jwt-cpp: jwt-cpp is
// linked PRIVATE into bsfchat_protocol, so it is not visible here. That is
// arguably better for a wire-format test — it asserts the bytes LiveKit will
// actually parse, not what the library that produced them thinks they mean.

namespace {

// Splits "header.payload.signature" and JSON-decodes the requested segment.
nlohmann::json decode_segment(const std::string& token, int index) {
    size_t start = 0;
    for (int i = 0; i < index; ++i) {
        start = token.find('.', start);
        if (start == std::string::npos) throw std::runtime_error("too few segments");
        ++start;
    }
    size_t end = token.find('.', start);
    const std::string seg = token.substr(start, end == std::string::npos ? std::string::npos : end - start);
    auto bytes = bsfchat::base64url_decode(seg);
    return nlohmann::json::parse(std::string(bytes.begin(), bytes.end()));
}

std::string signature_of(const std::string& token) {
    return token.substr(token.rfind('.') + 1);
}

bsfchat::LiveKitGrants basic_grants() {
    bsfchat::LiveKitGrants g;
    g.room = "bsfchat-room";
    return g;
}

constexpr const char* kKey = "APIabcdef123456";
constexpr const char* kSecret = "s3cr3t-livekit-signing-key-goes-here";

} // namespace

TEST(LiveKitToken, HeaderIsHs256Jwt) {
    auto token = livekit_token_sign(kKey, kSecret, "@alice:test", "", basic_grants(), 600, 1000);
    auto header = decode_segment(token, 0);
    EXPECT_EQ(header.value("alg", ""), "HS256");
    EXPECT_EQ(header.value("typ", ""), "JWT");
}

TEST(LiveKitToken, RegisteredClaims) {
    auto token = livekit_token_sign(kKey, kSecret, "@alice:test", "Alice", basic_grants(), 600, 1000);
    auto p = decode_segment(token, 1);

    // iss is the API key — it tells LiveKit which secret to verify against.
    EXPECT_EQ(p.value("iss", ""), kKey);
    // sub is the participant identity.
    EXPECT_EQ(p.value("sub", ""), "@alice:test");
    EXPECT_EQ(p.value("nbf", int64_t{0}), 1000);
    EXPECT_EQ(p.value("iat", int64_t{0}), 1000);
    EXPECT_EQ(p.value("exp", int64_t{0}), 1600);
    EXPECT_EQ(p.value("name", ""), "Alice");
}

TEST(LiveKitToken, DisplayNameOmittedWhenEmpty) {
    auto token = livekit_token_sign(kKey, kSecret, "@alice:test", "", basic_grants(), 600, 1000);
    EXPECT_FALSE(decode_segment(token, 1).contains("name"));
}

TEST(LiveKitToken, VideoGrantUsesLiveKitFieldNames) {
    LiveKitGrants g = basic_grants();
    g.room_admin = true;
    g.can_publish_data = true;
    g.hidden = true;
    g.can_publish_sources = {"microphone", "screen_share"};

    auto p = decode_segment(livekit_token_sign(kKey, kSecret, "@alice:test", "", g, 600, 1000), 1);
    ASSERT_TRUE(p.contains("video"));
    const auto& v = p["video"];

    EXPECT_EQ(v.value("room", ""), "bsfchat-room");
    EXPECT_TRUE(v.value("roomJoin", false));
    EXPECT_TRUE(v.value("roomAdmin", false));
    EXPECT_TRUE(v.value("canPublish", false));
    EXPECT_TRUE(v.value("canSubscribe", false));
    EXPECT_TRUE(v.value("canPublishData", false));
    EXPECT_TRUE(v.value("hidden", false));
    EXPECT_EQ(v["canPublishSources"],
              nlohmann::json::array({"microphone", "screen_share"}));
}

// LiveKit's VideoGrant declares canPublish/canSubscribe/canPublishData as
// *bool with `omitempty`: an ABSENT key means true, not false. So a denial is
// only a denial if the key is physically present and false. This is the single
// easiest way to accidentally grant publish rights, hence its own test.
TEST(LiveKitToken, DenialsAreEmittedExplicitlyNotOmitted) {
    LiveKitGrants g = basic_grants();
    g.can_publish = false;
    g.can_subscribe = false;
    g.can_publish_data = false;

    auto v = decode_segment(livekit_token_sign(kKey, kSecret, "@bob:test", "", g, 600, 1000), 1)["video"];

    ASSERT_TRUE(v.contains("canPublish"));
    ASSERT_TRUE(v.contains("canSubscribe"));
    ASSERT_TRUE(v.contains("canPublishData"));
    EXPECT_FALSE(v["canPublish"].get<bool>());
    EXPECT_FALSE(v["canSubscribe"].get<bool>());
    EXPECT_FALSE(v["canPublishData"].get<bool>());
}

// An empty source list means "no restriction" to LiveKit, so it must be
// omitted rather than sent as [] (which would restrict to nothing).
TEST(LiveKitToken, EmptyPublishSourcesOmitted) {
    auto v = decode_segment(livekit_token_sign(kKey, kSecret, "@a:test", "", basic_grants(), 600, 1000), 1)["video"];
    EXPECT_FALSE(v.contains("canPublishSources"));
}

TEST(LiveKitToken, SignatureIsBoundToTheSecret) {
    auto a = livekit_token_sign(kKey, kSecret, "@alice:test", "", basic_grants(), 600, 1000);
    auto b = livekit_token_sign(kKey, "a-completely-different-secret", "@alice:test", "",
                                basic_grants(), 600, 1000);

    // Same claims, so header and payload must match byte for byte...
    EXPECT_EQ(decode_segment(a, 0), decode_segment(b, 0));
    EXPECT_EQ(decode_segment(a, 1), decode_segment(b, 1));
    // ...and only the signature differs.
    EXPECT_NE(signature_of(a), signature_of(b));
}

TEST(LiveKitToken, DeterministicForFixedClock) {
    auto a = livekit_token_sign(kKey, kSecret, "@alice:test", "Alice", basic_grants(), 600, 1000);
    auto b = livekit_token_sign(kKey, kSecret, "@alice:test", "Alice", basic_grants(), 600, 1000);
    EXPECT_EQ(a, b);
}

TEST(LiveKitToken, SecretNeverAppearsInTheToken) {
    auto token = livekit_token_sign(kKey, kSecret, "@alice:test", "Alice", basic_grants(), 600, 1000);
    EXPECT_EQ(token.find(kSecret), std::string::npos);
    // Nor base64url-encoded anywhere in it.
    for (int seg = 0; seg < 2; ++seg) {
        EXPECT_EQ(decode_segment(token, seg).dump().find(kSecret), std::string::npos);
    }
}

TEST(LiveKitToken, TtlClampedToBounds) {
    auto too_short = decode_segment(
        livekit_token_sign(kKey, kSecret, "@a:test", "", basic_grants(), 1, 1000), 1);
    EXPECT_EQ(too_short.value("exp", int64_t{0}), 1000 + kLiveKitMinTtl);

    auto too_long = decode_segment(
        livekit_token_sign(kKey, kSecret, "@a:test", "", basic_grants(), 999999, 1000), 1);
    EXPECT_EQ(too_long.value("exp", int64_t{0}), 1000 + kLiveKitMaxTtl);

    auto negative = decode_segment(
        livekit_token_sign(kKey, kSecret, "@a:test", "", basic_grants(), -5, 1000), 1);
    EXPECT_EQ(negative.value("exp", int64_t{0}), 1000 + kLiveKitMinTtl);
}

TEST(LiveKitToken, RejectsMissingCredentialsOrIdentity) {
    EXPECT_THROW(livekit_token_sign("", kSecret, "@a:test", "", basic_grants(), 600, 1000),
                 std::invalid_argument);
    EXPECT_THROW(livekit_token_sign(kKey, "", "@a:test", "", basic_grants(), 600, 1000),
                 std::invalid_argument);
    EXPECT_THROW(livekit_token_sign(kKey, kSecret, "", "", basic_grants(), 600, 1000),
                 std::invalid_argument);
}

// A roomJoin grant with no room name is a wildcard over every room on the SFU.
TEST(LiveKitToken, RejectsJoinOrAdminGrantWithoutRoomName) {
    LiveKitGrants no_room;
    no_room.room_join = true;
    EXPECT_THROW(livekit_token_sign(kKey, kSecret, "@a:test", "", no_room, 600, 1000),
                 std::invalid_argument);

    LiveKitGrants admin_no_room;
    admin_no_room.room_join = false;
    admin_no_room.room_admin = true;
    EXPECT_THROW(livekit_token_sign(kKey, kSecret, "@a:test", "", admin_no_room, 600, 1000),
                 std::invalid_argument);
}
