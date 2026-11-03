#include "gg_contract.h"
#include "gg_raii.h"

#include <cstddef>

namespace {

unsigned char ascii_lower(unsigned char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<unsigned char>(c + ('a' - 'A')) : c;
}

} // namespace

int gg_verify_signature(const uint8_t *data, size_t length, const char *expected_name,
                        const uint8_t *public_blob, size_t public_blob_length) {
    gg_footer footer;
    if (!expected_name || !public_blob || public_blob_length > MAXDWORD ||
        !gg_parse_footer(data, length, &footer) || footer.signed_length > MAXDWORD)
        return 0;

    std::size_t i = 0;
    for (; i + 1 < footer.name_length; ++i) {
        if (!expected_name[i] || ascii_lower(static_cast<unsigned char>(expected_name[i])) !=
                                     ascii_lower(data[footer.name_offset + i]))
            return 0;
    }
    if (expected_name[i])
        return 0;

    gg::CryptProvider provider;
    gg::CryptKey key;
    gg::CryptHash hash;
    if (!CryptAcquireContextW(provider.receive(), nullptr, nullptr, PROV_RSA_AES,
                              CRYPT_VERIFYCONTEXT) ||
        !CryptImportKey(provider.get(), public_blob, static_cast<DWORD>(public_blob_length), 0, 0,
                        key.receive()) ||
        !CryptCreateHash(provider.get(), CALG_MD5, 0, 0, hash.receive()) ||
        !CryptHashData(hash.get(), data, static_cast<DWORD>(footer.signed_length), 0))
        return 0;
    return CryptVerifySignatureA(hash.get(), data + footer.signature_offset,
                                 static_cast<DWORD>(footer.signature_length), key.get(), nullptr, 0)
               ? 1
               : 0;
}
