#include "bsfchat/JwtUtils.h"

#include <jwt-cpp/jwt.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <memory>
#include <stdexcept>

namespace bsfchat {

namespace {

std::string base64url_encode(const unsigned char* data, size_t len) {
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    result.reserve((len * 4 + 2) / 3);

    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

        result += table[(n >> 18) & 0x3F];
        result += table[(n >> 12) & 0x3F];
        if (i + 1 < len) result += table[(n >> 6) & 0x3F];
        if (i + 2 < len) result += table[n & 0x3F];
    }
    return result;
}

} // namespace

std::string jwt_sign(
    const JwtClaims& claims,
    const std::string& pem_private_key,
    const std::string& key_id
) {
    auto token = jwt::create()
        .set_type("JWT")
        .set_key_id(key_id)
        .set_algorithm("RS256")
        .set_issuer(claims.iss)
        .set_subject(claims.sub)
        .set_audience(claims.aud)
        .set_issued_at(jwt::date(std::chrono::system_clock::from_time_t(claims.iat)))
        .set_expires_at(jwt::date(std::chrono::system_clock::from_time_t(claims.exp)));

    if (claims.name) token.set_payload_claim("name", jwt::claim(std::string(*claims.name)));
    if (claims.email) token.set_payload_claim("email", jwt::claim(std::string(*claims.email)));
    if (claims.picture) token.set_payload_claim("picture", jwt::claim(std::string(*claims.picture)));
    if (claims.azp) token.set_payload_claim("azp", jwt::claim(std::string(*claims.azp)));
    if (claims.nonce) token.set_payload_claim("nonce", jwt::claim(std::string(*claims.nonce)));

    return token.sign(jwt::algorithm::rs256("", pem_private_key));
}

std::optional<JwtClaims> jwt_verify(
    const std::string& token,
    const std::string& pem_public_key,
    const std::string& issuer,
    const std::string& expected_audience
) {
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::rs256(pem_public_key))
            .with_issuer(issuer)
            .leeway(60); // 60 seconds clock skew tolerance

        // Audience was previously never checked, so an ID token minted for ANY
        // other OAuth client registered with the same identity provider
        // verified successfully and was accepted as a chat login. An empty
        // expected_audience preserves the old (unchecked) behaviour for
        // callers that have no audience to assert.
        if (!expected_audience.empty()) {
            verifier = verifier.with_audience(expected_audience);
        }

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        // with_audience() only constrains tokens that carry an `aud` claim in
        // some jwt-cpp versions; require its presence explicitly so a token
        // minted without an audience cannot slip through.
        if (!expected_audience.empty() && !decoded.has_audience()) {
            return std::nullopt;
        }

        JwtClaims claims;
        claims.sub = decoded.get_subject();
        claims.iss = decoded.get_issuer();
        if (!decoded.get_audience().empty()) {
            claims.aud = *decoded.get_audience().begin();
        }
        claims.iat = std::chrono::system_clock::to_time_t(decoded.get_issued_at());
        claims.exp = std::chrono::system_clock::to_time_t(decoded.get_expires_at());

        if (decoded.has_payload_claim("name")) {
            claims.name = decoded.get_payload_claim("name").as_string();
        }
        if (decoded.has_payload_claim("email")) {
            claims.email = decoded.get_payload_claim("email").as_string();
        }
        if (decoded.has_payload_claim("picture")) {
            claims.picture = decoded.get_payload_claim("picture").as_string();
        }
        if (decoded.has_payload_claim("azp")) {
            claims.azp = decoded.get_payload_claim("azp").as_string();
        }
        if (decoded.has_payload_claim("nonce")) {
            claims.nonce = decoded.get_payload_claim("nonce").as_string();
        }

        return claims;
    } catch (...) {
        return std::nullopt;
    }
}

namespace {

bool is_ascii_digit(char c) { return c >= '0' && c <= '9'; }
bool is_ascii_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
char ascii_lower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

// A DNS name or IPv4 literal, already lower-cased: labels of [a-z0-9-], none
// empty, none starting or ending with '-'. Deliberately narrower than what a
// resolver accepts — an IDN arrives here as its punycode A-label, which is
// what the address bar, TLS and the chat server's own config all use.
bool host_is_acceptable(std::string_view host) {
    if (host.empty() || host.size() > 253) return false;
    size_t label_start = 0;
    for (size_t i = 0; i <= host.size(); ++i) {
        if (i == host.size() || host[i] == '.') {
            const size_t len = i - label_start;
            if (len == 0 || len > 63) return false;  // empty label, incl. trailing dot
            if (host[label_start] == '-' || host[i - 1] == '-') return false;
            label_start = i + 1;
            continue;
        }
        const char c = host[i];
        if (!(is_ascii_digit(c) || (c >= 'a' && c <= 'z') || c == '-')) return false;
    }
    return true;
}

bool ipv6_literal_is_acceptable(std::string_view inner) {
    // Contents of "[...]". Hex digits, ':' and '.' (for an embedded IPv4
    // tail). A zone id ("%25en0") is refused: it names an interface on the
    // machine doing the parsing, which is meaningless as an audience.
    if (inner.size() < 2 || inner.find(':') == std::string_view::npos) return false;
    for (char c : inner) {
        const bool hex = is_ascii_digit(c) || (c >= 'a' && c <= 'f');
        if (!(hex || c == ':' || c == '.')) return false;
    }
    return true;
}

bool path_char_is_acceptable(char c) {
    // RFC 3986 unreserved plus '/'. No '%': an escaped and an unescaped
    // spelling of one path would otherwise be two audiences.
    return is_ascii_alpha(c) || is_ascii_digit(c) || c == '-' || c == '.' || c == '_' ||
           c == '~' || c == '/';
}

} // namespace

std::optional<std::string> canonical_audience_url(std::string_view url) {
    if (url.empty() || url.size() > 512) return std::nullopt;
    for (char c : url) {
        // Controls, space, DEL and anything non-ASCII: none of them belong in
        // an audience, and each is a way to make two strings that render alike.
        if (static_cast<unsigned char>(c) <= 0x20 || static_cast<unsigned char>(c) >= 0x7f) {
            return std::nullopt;
        }
    }

    const auto sep = url.find("://");
    if (sep == std::string_view::npos) return std::nullopt;
    std::string scheme;
    for (char c : url.substr(0, sep)) scheme += ascii_lower(c);
    if (scheme != "https" && scheme != "http") return std::nullopt;

    std::string_view rest = url.substr(sep + 3);
    if (rest.find_first_of("?#") != std::string_view::npos) return std::nullopt;

    const auto slash = rest.find('/');
    std::string_view authority = rest.substr(0, slash);
    std::string_view path = slash == std::string_view::npos ? std::string_view{} : rest.substr(slash);

    if (authority.empty() || authority.find('@') != std::string_view::npos) return std::nullopt;

    std::string authority_lower;
    for (char c : authority) authority_lower += ascii_lower(c);
    std::string_view auth = authority_lower;

    std::string host;
    std::string_view port_str;
    if (auth.front() == '[') {
        const auto close = auth.find(']');
        if (close == std::string_view::npos) return std::nullopt;
        if (!ipv6_literal_is_acceptable(auth.substr(1, close - 1))) return std::nullopt;
        host = std::string(auth.substr(0, close + 1));
        std::string_view after = auth.substr(close + 1);
        if (!after.empty()) {
            if (after.front() != ':') return std::nullopt;
            port_str = after.substr(1);
            if (port_str.empty()) return std::nullopt;
        }
    } else {
        const auto colon = auth.find(':');
        host = std::string(auth.substr(0, colon));
        if (colon != std::string_view::npos) {
            port_str = auth.substr(colon + 1);
            if (port_str.empty()) return std::nullopt;
        }
        if (!host_is_acceptable(host)) return std::nullopt;
    }

    std::string port;
    if (!port_str.empty()) {
        if (port_str.size() > 5) return std::nullopt;
        uint32_t value = 0;
        for (char c : port_str) {
            if (!is_ascii_digit(c)) return std::nullopt;
            value = value * 10 + static_cast<uint32_t>(c - '0');
        }
        if (value == 0 || value > 65535) return std::nullopt;
        const bool is_default = (scheme == "https" && value == 443) || (scheme == "http" && value == 80);
        if (!is_default) port = std::to_string(value);  // also drops leading zeros
    }

    // Trailing slashes carry no meaning for a base URL; everything else in
    // the path must already be in its one canonical spelling.
    while (!path.empty() && path.back() == '/') path.remove_suffix(1);
    if (!path.empty()) {
        for (char c : path) {
            if (!path_char_is_acceptable(c)) return std::nullopt;
        }
        size_t pos = 1;  // path[0] is '/'
        while (pos <= path.size()) {
            auto next = path.find('/', pos);
            if (next == std::string_view::npos) next = path.size();
            std::string_view segment = path.substr(pos, next - pos);
            if (segment.empty() || segment == "." || segment == "..") return std::nullopt;
            pos = next + 1;
        }
    }

    std::string out = scheme + "://" + host;
    if (!port.empty()) out += ":" + port;
    out += path;
    return out;
}

bool audience_url_is_secure(std::string_view canonical_url) {
    if (canonical_url.starts_with("https://")) return true;
    if (!canonical_url.starts_with("http://")) return false;
    std::string_view rest = canonical_url.substr(7);
    std::string_view host;
    if (rest.starts_with("[")) {
        host = rest.substr(0, rest.find(']') == std::string_view::npos ? 0 : rest.find(']') + 1);
    } else {
        host = rest.substr(0, rest.find_first_of(":/"));
    }
    if (host == "localhost" || host == "[::1]") return true;
    // 127.0.0.0/8, written as a dotted quad. host_is_acceptable() has
    // already limited it to [a-z0-9.-], so a "127." prefix plus three more
    // all-digit labels is an IPv4 loopback literal and nothing else.
    if (host.starts_with("127.")) {
        int labels = 0;
        size_t start = 0;
        for (size_t i = 0; i <= host.size(); ++i) {
            if (i == host.size() || host[i] == '.') {
                std::string_view label = host.substr(start, i - start);
                if (label.empty() || label.size() > 3) return false;
                int value = 0;
                for (char c : label) {
                    if (!is_ascii_digit(c)) return false;
                    value = value * 10 + (c - '0');
                }
                if (value > 255) return false;
                ++labels;
                start = i + 1;
            }
        }
        return labels == 4;
    }
    return false;
}

std::string livekit_token_sign(
    const std::string& api_key,
    const std::string& api_secret,
    const std::string& identity,
    const std::string& display_name,
    const LiveKitGrants& grants,
    int64_t ttl_seconds,
    int64_t now_unix
) {
    if (api_key.empty()) throw std::invalid_argument("livekit_token_sign: empty api_key");
    if (api_secret.empty()) throw std::invalid_argument("livekit_token_sign: empty api_secret");
    if (identity.empty()) throw std::invalid_argument("livekit_token_sign: empty identity");
    // A join/admin grant with no room name is a room-wildcard as far as
    // LiveKit is concerned. Refuse to mint one rather than hand out a token
    // that is broader than any caller intended.
    if ((grants.room_join || grants.room_admin) && grants.room.empty()) {
        throw std::invalid_argument("livekit_token_sign: room_join/room_admin requires a room name");
    }

    if (ttl_seconds < kLiveKitMinTtl) ttl_seconds = kLiveKitMinTtl;
    if (ttl_seconds > kLiveKitMaxTtl) ttl_seconds = kLiveKitMaxTtl;

    const int64_t now = now_unix > 0
        ? now_unix
        : std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch()).count();

    // The `video` grant. Keys are LiveKit's JSON tags — see LiveKitGrants.
    picojson::object video;
    video["room"] = picojson::value(grants.room);
    video["roomJoin"] = picojson::value(grants.room_join);
    video["roomAdmin"] = picojson::value(grants.room_admin);
    video["canPublish"] = picojson::value(grants.can_publish);
    video["canSubscribe"] = picojson::value(grants.can_subscribe);
    video["canPublishData"] = picojson::value(grants.can_publish_data);
    video["hidden"] = picojson::value(grants.hidden);
    if (!grants.can_publish_sources.empty()) {
        picojson::array sources;
        sources.reserve(grants.can_publish_sources.size());
        for (const auto& s : grants.can_publish_sources) {
            sources.emplace_back(s);
        }
        video["canPublishSources"] = picojson::value(sources);
    }

    auto builder = jwt::create()
        .set_type("JWT")
        .set_issuer(api_key)
        .set_subject(identity)
        .set_issued_at(jwt::date(std::chrono::system_clock::from_time_t(static_cast<time_t>(now))))
        .set_not_before(jwt::date(std::chrono::system_clock::from_time_t(static_cast<time_t>(now))))
        .set_expires_at(jwt::date(std::chrono::system_clock::from_time_t(
            static_cast<time_t>(now + ttl_seconds))))
        .set_payload_claim("video", jwt::claim(picojson::value(video)));

    // LiveKit also reads `identity` from the grants object in some code paths;
    // `sub` is the canonical one and is what its own server SDKs set.
    if (!display_name.empty()) {
        builder = builder.set_payload_claim("name", jwt::claim(display_name));
    }

    return builder.sign(jwt::algorithm::hs256(api_secret));
}

nlohmann::json pem_to_jwk(const std::string& pem_public_key, const std::string& key_id) {
    // Parse the PEM public key
    auto bio = std::unique_ptr<BIO, decltype(&BIO_free)>(
        BIO_new_mem_buf(pem_public_key.data(), static_cast<int>(pem_public_key.size())),
        BIO_free
    );

    auto pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(
        PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr),
        EVP_PKEY_free
    );

    if (!pkey) throw std::runtime_error("Failed to parse PEM public key");

    BIGNUM* n_bn = nullptr;
    BIGNUM* e_bn = nullptr;
    if (!EVP_PKEY_get_bn_param(pkey.get(), "n", &n_bn) ||
        !EVP_PKEY_get_bn_param(pkey.get(), "e", &e_bn)) {
        if (n_bn) BN_free(n_bn);
        if (e_bn) BN_free(e_bn);
        throw std::runtime_error("Failed to extract RSA parameters");
    }

    auto n_cleanup = std::unique_ptr<BIGNUM, decltype(&BN_free)>(n_bn, BN_free);
    auto e_cleanup = std::unique_ptr<BIGNUM, decltype(&BN_free)>(e_bn, BN_free);

    // Convert BIGNUM to base64url
    std::vector<unsigned char> n_bytes(BN_num_bytes(n_bn));
    std::vector<unsigned char> e_bytes(BN_num_bytes(e_bn));
    BN_bn2bin(n_bn, n_bytes.data());
    BN_bn2bin(e_bn, e_bytes.data());

    return {
        {"kty", "RSA"},
        {"use", "sig"},
        {"alg", "RS256"},
        {"kid", key_id},
        {"n", base64url_encode(n_bytes.data(), n_bytes.size())},
        {"e", base64url_encode(e_bytes.data(), e_bytes.size())},
    };
}

std::vector<unsigned char> base64url_decode(const std::string& input) {
    // Replace base64url chars with standard base64
    std::string base64 = input;
    for (auto& c : base64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding
    while (base64.size() % 4 != 0) base64 += '=';

    // Decode using OpenSSL BIO
    auto bio = std::unique_ptr<BIO, decltype(&BIO_free_all)>(
        BIO_new_mem_buf(base64.data(), static_cast<int>(base64.size())),
        BIO_free_all
    );
    auto b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio.reset(BIO_push(b64, bio.release()));

    std::vector<unsigned char> result(base64.size()); // output is always <= input
    int len = BIO_read(bio.get(), result.data(), static_cast<int>(result.size()));
    if (len < 0) len = 0;
    result.resize(static_cast<size_t>(len));
    return result;
}

std::string jwk_to_pem(const nlohmann::json& jwk) {
    if (jwk.value("kty", "") != "RSA") {
        throw std::runtime_error("jwk_to_pem: only RSA keys are supported");
    }

    auto n_bytes = base64url_decode(jwk.at("n").get<std::string>());
    auto e_bytes = base64url_decode(jwk.at("e").get<std::string>());

    auto n_bn = std::unique_ptr<BIGNUM, decltype(&BN_free)>(
        BN_bin2bn(n_bytes.data(), static_cast<int>(n_bytes.size()), nullptr), BN_free);
    auto e_bn = std::unique_ptr<BIGNUM, decltype(&BN_free)>(
        BN_bin2bn(e_bytes.data(), static_cast<int>(e_bytes.size()), nullptr), BN_free);

    if (!n_bn || !e_bn) {
        throw std::runtime_error("jwk_to_pem: failed to create BIGNUMs");
    }

    // Build EVP_PKEY using OpenSSL 3.x OSSL_PARAM_BLD
    auto bld = std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)>(
        OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
    if (!bld ||
        !OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_N, n_bn.get()) ||
        !OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_E, e_bn.get())) {
        throw std::runtime_error("jwk_to_pem: failed to build params");
    }

    auto params = std::unique_ptr<OSSL_PARAM, decltype(&OSSL_PARAM_free)>(
        OSSL_PARAM_BLD_to_param(bld.get()), OSSL_PARAM_free);
    if (!params) {
        throw std::runtime_error("jwk_to_pem: failed to create params");
    }

    auto ctx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>(
        EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) <= 0) {
        throw std::runtime_error("jwk_to_pem: failed to init fromdata");
    }

    EVP_PKEY* pkey_raw = nullptr;
    if (EVP_PKEY_fromdata(ctx.get(), &pkey_raw, EVP_PKEY_PUBLIC_KEY, params.get()) <= 0) {
        throw std::runtime_error("jwk_to_pem: failed to create key from data");
    }
    auto pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(pkey_raw, EVP_PKEY_free);

    // Write to PEM
    auto bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new(BIO_s_mem()), BIO_free);
    if (!PEM_write_bio_PUBKEY(bio.get(), pkey.get())) {
        throw std::runtime_error("jwk_to_pem: failed to write PEM");
    }

    char* pem_data = nullptr;
    long pem_len = BIO_get_mem_data(bio.get(), &pem_data);
    return std::string(pem_data, pem_len);
}

std::pair<std::string, std::string> generate_rsa_keypair() {
    auto ctx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>(
        EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr),
        EVP_PKEY_CTX_free
    );

    if (!ctx || EVP_PKEY_keygen_init(ctx.get()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0) {
        throw std::runtime_error("Failed to initialize RSA key generation");
    }

    EVP_PKEY* pkey_raw = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &pkey_raw) <= 0) {
        throw std::runtime_error("Failed to generate RSA key pair");
    }
    auto pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(pkey_raw, EVP_PKEY_free);

    // Write private key
    auto priv_bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new(BIO_s_mem()), BIO_free);
    PEM_write_bio_PrivateKey(priv_bio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr);
    char* priv_data = nullptr;
    long priv_len = BIO_get_mem_data(priv_bio.get(), &priv_data);
    std::string private_pem(priv_data, priv_len);

    // Write public key
    auto pub_bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new(BIO_s_mem()), BIO_free);
    PEM_write_bio_PUBKEY(pub_bio.get(), pkey.get());
    char* pub_data = nullptr;
    long pub_len = BIO_get_mem_data(pub_bio.get(), &pub_data);
    std::string public_pem(pub_data, pub_len);

    return {private_pem, public_pem};
}

} // namespace bsfchat
