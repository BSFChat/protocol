#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

namespace bsfchat {

// JWT utilities for OIDC identity token handling.
// Used by the identity service to sign tokens and by chat servers to verify them.

struct JwtClaims {
    std::string sub;       // subject (user ID)
    std::string iss;       // issuer (identity service URL)
    std::string aud;       // audience (chat server client_id)
    int64_t iat = 0;       // issued at (unix timestamp)
    int64_t exp = 0;       // expiry (unix timestamp)
    std::optional<std::string> name;
    std::optional<std::string> email;
    std::optional<std::string> picture;
};

// Create a signed JWT using RS256.
// pem_private_key: RSA private key in PEM format.
// key_id: the "kid" header value for JWKS matching.
std::string jwt_sign(
    const JwtClaims& claims,
    const std::string& pem_private_key,
    const std::string& key_id
);

// Verify a JWT and extract claims.
// pem_public_key: RSA public key in PEM format.
// issuer: expected issuer claim.
// Returns nullopt if verification fails (bad signature, expired, wrong issuer).
std::optional<JwtClaims> jwt_verify(
    const std::string& token,
    const std::string& pem_public_key,
    const std::string& issuer
);

// Convert an RSA public key (PEM) to a JWK (JSON Web Key) for the JWKS endpoint.
nlohmann::json pem_to_jwk(const std::string& pem_public_key, const std::string& key_id);

// Convert a JWK (JSON Web Key) to an RSA public key in PEM format.
std::string jwk_to_pem(const nlohmann::json& jwk);

// Decode a base64url-encoded string.
std::vector<unsigned char> base64url_decode(const std::string& input);

// Generate an RSA key pair (2048 bits). Returns {private_pem, public_pem}.
std::pair<std::string, std::string> generate_rsa_keypair();

} // namespace bsfchat
