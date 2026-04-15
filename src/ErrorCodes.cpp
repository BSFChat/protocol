#include "bsfchat/ErrorCodes.h"

namespace bsfchat {

nlohmann::json MatrixError::to_json() const {
    return {{"errcode", errcode}, {"error", error}};
}

MatrixError MatrixError::from_json(const nlohmann::json& j) {
    return {
        .errcode = j.at("errcode").get<std::string>(),
        .error = j.value("error", ""),
    };
}

MatrixError MatrixError::forbidden(const std::string& msg) {
    return {.errcode = std::string(error_code::kForbidden), .error = msg};
}

MatrixError MatrixError::unknown_token(const std::string& msg) {
    return {.errcode = std::string(error_code::kUnknownToken), .error = msg};
}

MatrixError MatrixError::missing_token(const std::string& msg) {
    return {.errcode = std::string(error_code::kMissingToken), .error = msg};
}

MatrixError MatrixError::bad_json(const std::string& msg) {
    return {.errcode = std::string(error_code::kBadJson), .error = msg};
}

MatrixError MatrixError::not_found(const std::string& msg) {
    return {.errcode = std::string(error_code::kNotFound), .error = msg};
}

MatrixError MatrixError::user_in_use(const std::string& msg) {
    return {.errcode = std::string(error_code::kUserInUse), .error = msg};
}

MatrixError MatrixError::invalid_username(const std::string& msg) {
    return {.errcode = std::string(error_code::kInvalidUsername), .error = msg};
}

MatrixError MatrixError::unknown(const std::string& msg) {
    return {.errcode = std::string(error_code::kUnknown), .error = msg};
}

MatrixError MatrixError::invalid_param(const std::string& msg) {
    return {.errcode = std::string(error_code::kInvalidParam), .error = msg};
}

MatrixError MatrixError::too_large(const std::string& msg) {
    return {.errcode = std::string(error_code::kTooLarge), .error = msg};
}

MatrixError MatrixError::unrecognized(const std::string& msg) {
    return {.errcode = std::string(error_code::kUnrecognized), .error = msg};
}

} // namespace bsfchat
