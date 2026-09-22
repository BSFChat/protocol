#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>

namespace bsfchat {

// JWT utilities for OIDC identity token handling.
// Used by the identity service to sign tokens and by chat servers to verify them.

struct JwtClaims {
    std::string sub;       // subject (user ID)
    std::string iss;       // issuer (identity service URL)
    // Audience. For an id_token that a chat server will accept as a sign-in
    // credential this is the chat server's own public URL, in the form
    // canonical_audience_url() produces — NOT the OAuth client_id.
    //
    // It used to be the client_id ("bsfchat-desktop") for every token, which
    // made one id_token a sign-in credential on every chat server trusting the
    // same provider: a hostile server that received a user's token at sign-in
    // could replay it anywhere else (identity audit 2026-09, finding C1). The
    // client now names the server it is signing in to as an RFC 8707
    // `resource`, the provider puts that here, and each chat server requires
    // its own URL.
    std::string aud;
    int64_t iat = 0;       // issued at (unix timestamp)
    int64_t exp = 0;       // expiry (unix timestamp)
    std::optional<std::string> name;
    std::optional<std::string> email;
    std::optional<std::string> picture;
    // Authorized party (OIDC Core 2): the OAuth client the token was issued
    // to. Once `aud` names the chat server rather than the client, this is
    // where the client_id lives.
    std::optional<std::string> azp;
    // OIDC Core 3.1.2.1 nonce, echoed from the authorization request. The
    // desktop client checks it against the value it generated for that
    // attempt; a chat server uses it to refuse a second presentation of the
    // same token.
    std::optional<std::string> nonce;
};

// Canonical form of a URL used as a token audience (an RFC 8707 resource
// indicator naming a chat server). Three parties spell the same server
// independently — the client from the address it connects to, the identity
// provider when it writes `aud`, the chat server from its own configuration —
// and jwt-cpp compares audiences byte for byte, so all three must pass
// through this one function or a correctly configured deployment fails
// closed on "https://Chat.Example:443/" versus "https://chat.example".
//
//   scheme      http or https, lower-cased
//   host        lower-cased; a DNS name, an IPv4 literal or a bracketed IPv6
//               literal. No userinfo ("https://real.example@evil.example" is
//               a classic confusion), no trailing dot, no empty labels.
//   port        dropped when it is the scheme's default, else kept (1-65535)
//   path        kept, minus trailing slashes; "." and ".." segments, empty
//               segments and percent-escapes are refused rather than
//               normalised, so there is exactly one spelling of a path
//   query/frag  refused
//
// Returns nullopt for anything else. Purely syntactic: whether an http URL is
// acceptable is policy, decided by the caller (see audience_url_is_secure).
std::optional<std::string> canonical_audience_url(std::string_view url);

// True when a CANONICAL audience URL is https, or http to a loopback host
// (localhost, 127.0.0.0/8, [::1]) for development. The identity provider
// refuses to mint a token for anything else.
bool audience_url_is_secure(std::string_view canonical_url);

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
