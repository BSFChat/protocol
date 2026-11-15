#include "bsfchat/Permissions.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace bsfchat {
namespace permission {

std::string flags_to_hex(Flags flags) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(flags));
    return buf;
}

std::optional<Flags> try_flags_from_hex(std::string_view v) {
    // Strict: an optional "0x"/"0X" and then 1-16 hex digits, nothing else.
    // strtoull accepted leading whitespace, a sign, trailing junk and overflow,
    // which made "-1", "0x-1" and any too-long value parse as ALL ONES —
    // ADMINISTRATOR included — and "1ffzz" as 0x1ff, against the documented
    // "returns 0 on failure". A permission mask is the last field that should
    // be parsed generously.
    if (v.size() >= 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) v.remove_prefix(2);
    if (v.empty() || v.size() > 16) return std::nullopt;
    Flags out = 0;
    for (const char c : v) {
        Flags d;
        if (c >= '0' && c <= '9') d = static_cast<Flags>(c - '0');
        else if (c >= 'a' && c <= 'f') d = static_cast<Flags>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = static_cast<Flags>(c - 'A' + 10);
        else return std::nullopt;
        out = (out << 4) | d;
    }
    return out;
}

Flags flags_from_hex(const std::string& s) {
    return try_flags_from_hex(s).value_or(0);
}

} // namespace permission
} // namespace bsfchat
