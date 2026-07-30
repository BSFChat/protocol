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
// expected_audience: expected `aud` claim (the relying party's client_id).
//   When non-empty, a token whose audience differs — or that carries no
//   audience at all — is rejected. Without this an ID token minted for any
//   other client registered with the same provider was accepted.
//   Empty (the default) skips the audience check, preserving the previous
//   behaviour for callers that have no audience to assert.
// Returns nullopt if verification fails (bad signature, expired, wrong
// issuer, wrong audience).
std::optional<JwtClaims> jwt_verify(
    const std::string& token,
    const std::string& pem_public_key,
    const std::string& issuer,
    const std::string& expected_audience = std::string()
);

// ---------------------------------------------------------------------------
// LiveKit access tokens
// ---------------------------------------------------------------------------
//
// LiveKit (the SFU) authenticates participants with a JWT that is NOT the same
// shape as the OIDC tokens above:
//
//   * HS256 over a shared API secret, not RS256 over an RSA key. The
//     `iss` claim carries the API *key* that identifies which secret signed it.
//   * The permission set lives in a nested object claim, `video`, rather than
//     in flat string claims. JwtClaims cannot express that, which is why this
//     is a separate function rather than a flag on jwt_sign().
//   * `sub` is the participant identity. Two connections presenting the same
//     identity are treated as the same participant, and LiveKit disconnects
//     the older one — so identity must be unique per *device*, not per user.
//
// Field names below map 1:1 onto livekit/protocol's auth.VideoGrant JSON tags.
// Do not rename them.
struct LiveKitGrants {
    // Room name. Required whenever room_join or room_admin is set.
    std::string room;
    bool room_join = true;
    // Moderation of this room (server-side mute, participant removal).
    bool room_admin = false;
    // NOTE: LiveKit's VideoGrant declares canPublish/canSubscribe/
    // canPublishData as *bool with `omitempty`, so an ABSENT key means
    // "true" server-side, not "false". These are therefore always emitted
    // explicitly — dropping one to deny it would grant it instead.
    bool can_publish = true;
    bool can_subscribe = true;
    bool can_publish_data = false;
    // Restricts what can_publish covers. LiveKit accepts "camera",
    // "microphone", "screen_share" and "screen_share_audio". An empty list
    // means "no restriction", i.e. every source.
    std::vector<std::string> can_publish_sources;
    // Participant is in the room but invisible to other participants.
    bool hidden = false;
};

// Mint a LiveKit access token (HS256).
//
// api_key/api_secret: the LiveKit credential pair. api_key becomes `iss`;
//   api_secret is the HMAC key and must never be logged or returned.
// identity: participant identity -> `sub`. Must be unique per connection.
// display_name: optional `name` claim; pass an empty string to omit it.
// ttl_seconds: lifetime. Clamped to [kLiveKitMinTtl, kLiveKitMaxTtl].
// now_unix: injectable clock for tests; 0 means "use the system clock".
//
// Throws std::invalid_argument if api_key, api_secret or identity is empty,
// or if a join/admin grant carries no room name — every one of those would
// otherwise produce a token that is silently useless or dangerously broad.
std::string livekit_token_sign(
    const std::string& api_key,
    const std::string& api_secret,
    const std::string& identity,
    const std::string& display_name,
    const LiveKitGrants& grants,
    int64_t ttl_seconds,
    int64_t now_unix = 0
);

inline constexpr int64_t kLiveKitMinTtl = 30;          // 30 seconds
inline constexpr int64_t kLiveKitMaxTtl = 6 * 60 * 60; // 6 hours

// Convert an RSA public key (PEM) to a JWK (JSON Web Key) for the JWKS endpoint.
nlohmann::json pem_to_jwk(const std::string& pem_public_key, const std::string& key_id);

// Convert a JWK (JSON Web Key) to an RSA public key in PEM format.
std::string jwk_to_pem(const nlohmann::json& jwk);

// Decode a base64url-encoded string.
std::vector<unsigned char> base64url_decode(const std::string& input);

// Generate an RSA key pair (2048 bits). Returns {private_pem, public_pem}.
std::pair<std::string, std::string> generate_rsa_keypair();

} // namespace bsfchat
