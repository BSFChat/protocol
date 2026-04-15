#include "bsfchat/Permissions.h"

#include <cstdio>
#include <cstdlib>

namespace bsfchat {
namespace permission {

std::string flags_to_hex(Flags flags) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(flags));
    return buf;
}

Flags flags_from_hex(const std::string& s) {
    if (s.empty()) return 0;
    const char* p = s.c_str();
    // Tolerate leading "0x"/"0X".
    if (s.size() >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    char* end = nullptr;
    unsigned long long v = std::strtoull(p, &end, 16);
    if (end == p) return 0;
    return static_cast<Flags>(v);
}

} // namespace permission
} // namespace bsfchat
