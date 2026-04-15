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

struct MatrixError {
    std::string errcode;
    std::string error;

    [[nodiscard]] nlohmann::json to_json() const;
    static MatrixError from_json(const nlohmann::json& j);

    static MatrixError forbidden(const std::string& msg = "Forbidden");
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
};

} // namespace bsfchat
