#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace bsfchat {

// Parses and validates Matrix identifiers:
//   @user:server.com   (user ID)
//   !room:server.com   (room ID)
//   $eventid           (event ID)
//   #alias:server.com  (room alias)

struct UserId {
    std::string localpart;   // "alice"
    std::string server_name; // "server.com"

    [[nodiscard]] std::string to_string() const;
    static std::optional<UserId> parse(std::string_view input);
    static bool is_valid(std::string_view input);

    bool operator==(const UserId&) const = default;
};

struct RoomId {
    std::string localpart;   // random opaque string
    std::string server_name; // server that created it

    [[nodiscard]] std::string to_string() const;
    static std::optional<RoomId> parse(std::string_view input);
    static bool is_valid(std::string_view input);

    bool operator==(const RoomId&) const = default;
};

struct RoomAlias {
    std::string localpart;   // "general"
    std::string server_name; // "server.com"

    [[nodiscard]] std::string to_string() const;
    static std::optional<RoomAlias> parse(std::string_view input);
    static bool is_valid(std::string_view input);

    bool operator==(const RoomAlias&) const = default;
};

struct EventId {
    std::string opaque_id; // the full event ID after $

    [[nodiscard]] std::string to_string() const;
    static std::optional<EventId> parse(std::string_view input);
    static bool is_valid(std::string_view input);

    bool operator==(const EventId&) const = default;
};

// Generate a random event ID: $<base64>:server_name
std::string generate_event_id(std::string_view server_name);

// Generate a random room ID: !<base64>:server_name
std::string generate_room_id(std::string_view server_name);

// Every generator below draws from the OpenSSL CSPRNG. They used to draw from
// an mt19937 seeded with a single 32-bit value, which capped an access token at
// 2^32 possible values however long it looked; see random_base64() in
// Identifiers.cpp for the measurement and the reasoning.
//
// Consumers that depend on that property — the server stores access and refresh
// tokens as bearer secrets — should guard on this macro, so that building
// against a protocol checkout from before the fix fails at compile time instead
// of silently issuing guessable tokens. A build that links the old library is
// the failure mode that would otherwise reach production unnoticed: the server
// falls back to fetching protocol `main` from GitHub when no local checkout is
// present, so "it compiled" is not evidence of which version it got.
#define BSFCHAT_PROTOCOL_CSPRNG_IDENTIFIERS 1

// Generate a random access token: 43 base64url characters, ~256 bits.
std::string generate_access_token();

// Generate a random device ID
std::string generate_device_id();

// Generate a media ID
std::string generate_media_id();

} // namespace bsfchat
