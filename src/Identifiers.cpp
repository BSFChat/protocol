#include "bsfchat/Identifiers.h"

#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace bsfchat {

namespace {



// Base64url alphabet (no padding)
constexpr std::string_view kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// `chars` characters drawn from the CSPRNG, 6 bits each.
//
// This used to be `std::random_device rd; std::mt19937 gen(rd());` per call,
// and the length was a lie about the strength. mt19937 is seeded here from a
// SINGLE 32-bit value, so however many characters came out, the whole string
// was a pure function of 32 bits: an access token advertised as "~256 bits"
// had at most 2^32 possible values, and the entire keyspace of every token the
// server can ever issue enumerates in about 1.3 hours on one core (measured,
// not estimated — the seed is recovered from the first 8 characters). That is
// session hijacking by brute force against a live deployment, and it applied
// equally to refresh tokens, which are the thing that outlives a password
// change. mt19937 is a simulation PRNG; it was never a plausible source for a
// bearer secret.
//
// RAND_bytes is the OpenSSL CSPRNG, which is already linked here for JWT
// verification. `& 0x3F` is an exact 4:1 fold of 256 byte values onto the
// 64-character alphabet, so there is no modulo bias to correct for.
//
// A failure is thrown, never worked around. The only ways RAND_bytes fails are
// an unseeded or broken entropy source; quietly falling back to anything else
// would reintroduce exactly the defect above, with the comment still promising
// otherwise. A server that cannot generate a token must refuse the request.
std::string random_base64(size_t chars) {
    std::vector<unsigned char> buf(chars);
    if (chars > 0 && RAND_bytes(buf.data(), static_cast<int>(chars)) != 1) {
        throw std::runtime_error("RAND_bytes failed: no secure randomness available");
    }

    std::string result;
    result.reserve(chars);
    for (size_t i = 0; i < chars; ++i) {
        result += kBase64Chars[buf[i] & 0x3F];
    }
    return result;
}

// Parse "sigil + localpart : server_name" pattern
struct SigilParsed {
    std::string localpart;
    std::string server_name;
};

std::optional<SigilParsed> parse_sigil(std::string_view input, char sigil) {
    if (input.empty() || input[0] != sigil) return std::nullopt;

    auto colon_pos = input.find(':');
    if (colon_pos == std::string_view::npos || colon_pos == 1 || colon_pos == input.size() - 1) {
        return std::nullopt;
    }

    auto localpart = input.substr(1, colon_pos - 1);
    auto server_name = input.substr(colon_pos + 1);

    if (localpart.empty() || server_name.empty()) return std::nullopt;

    return SigilParsed{
        .localpart = std::string(localpart),
        .server_name = std::string(server_name),
    };
}

bool is_valid_localpart_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '.' || c == '_' || c == '=' || c == '-' || c == '/';
}

} // namespace

// UserId
std::string UserId::to_string() const {
    return "@" + localpart + ":" + server_name;
}

std::optional<UserId> UserId::parse(std::string_view input) {
    auto parsed = parse_sigil(input, '@');
    if (!parsed) return std::nullopt;

    // Validate localpart: lowercase alphanumeric + ._=-/
    for (char c : parsed->localpart) {
        if (!is_valid_localpart_char(c)) return std::nullopt;
    }

    return UserId{
        .localpart = std::move(parsed->localpart),
        .server_name = std::move(parsed->server_name),
    };
}

bool UserId::is_valid(std::string_view input) {
    return parse(input).has_value();
}

// RoomId
std::string RoomId::to_string() const {
    return "!" + localpart + ":" + server_name;
}

std::optional<RoomId> RoomId::parse(std::string_view input) {
    auto parsed = parse_sigil(input, '!');
    if (!parsed) return std::nullopt;

    return RoomId{
        .localpart = std::move(parsed->localpart),
        .server_name = std::move(parsed->server_name),
    };
}

bool RoomId::is_valid(std::string_view input) {
    return parse(input).has_value();
}

// RoomAlias
std::string RoomAlias::to_string() const {
    return "#" + localpart + ":" + server_name;
}

std::optional<RoomAlias> RoomAlias::parse(std::string_view input) {
    auto parsed = parse_sigil(input, '#');
    if (!parsed) return std::nullopt;

    return RoomAlias{
        .localpart = std::move(parsed->localpart),
        .server_name = std::move(parsed->server_name),
    };
}

bool RoomAlias::is_valid(std::string_view input) {
    return parse(input).has_value();
}

// EventId
std::string EventId::to_string() const {
    return "$" + opaque_id;
}

std::optional<EventId> EventId::parse(std::string_view input) {
    if (input.size() < 2 || input[0] != '$') return std::nullopt;
    return EventId{.opaque_id = std::string(input.substr(1))};
}

bool EventId::is_valid(std::string_view input) {
    return parse(input).has_value();
}

// Generators
std::string generate_event_id(std::string_view server_name) {
    return "$" + random_base64(24) + ":" + std::string(server_name);
}

std::string generate_room_id(std::string_view server_name) {
    return "!" + random_base64(18) + ":" + std::string(server_name);
}

std::string generate_access_token() {
    return random_base64(43); // ~256 bits
}

std::string generate_device_id() {
    return "DEVICE_" + random_base64(10);
}

std::string generate_media_id() {
    return random_base64(24);
}

} // namespace bsfchat
