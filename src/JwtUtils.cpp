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

    return token.sign(jwt::algorithm::rs256("", pem_private_key));
}

std::optional<JwtClaims> jwt_verify(
    const std::string& token,
    const std::string& pem_public_key,
    const std::string& issuer
) {
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::rs256(pem_public_key))
            .with_issuer(issuer)
            .leeway(60); // 60 seconds clock skew tolerance

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

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

        return claims;
    } catch (...) {
        return std::nullopt;
    }
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
