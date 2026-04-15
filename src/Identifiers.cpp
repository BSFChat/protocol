#include "bsfchat/Identifiers.h"

#include <algorithm>
#include <array>
#include <random>

namespace bsfchat {

namespace {



// Base64url alphabet (no padding)
constexpr std::string_view kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string random_base64(size_t bytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 63);

    std::string result;
    result.reserve(bytes);
    for (size_t i = 0; i < bytes; ++i) {
        result += kBase64Chars[dist(gen)];
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
