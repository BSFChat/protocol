#include <gtest/gtest.h>
#include "bsfchat/JwtUtils.h"

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
