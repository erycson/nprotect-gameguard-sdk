#include "gg_ini.h"
#include "gg_contract.h"
#include "gg_raii.h"

#include <cstring>

int gg_decrypt_ini_payload(const uint8_t *encrypted, size_t length, uint8_t *output,
                           size_t output_capacity, size_t *output_length) {
    if (!output_length)
        return 0;
    *output_length = 0;
    if (!encrypted || !output || !length || length > MAXDWORD || output_capacity < length)
        return 0;

    static const BYTE secret[] = "ectGameMon";
    gg::CryptProvider provider;
    gg::CryptHash hash;
    gg::CryptKey key;
    if (!CryptAcquireContextA(provider.receive(), nullptr, MS_DEF_PROV_A, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider.get(), CALG_MD5, 0, 0, hash.receive()) ||
        !CryptHashData(hash.get(), secret, static_cast<DWORD>(sizeof secret - 1u), 0) ||
        !CryptDeriveKey(provider.get(), CALG_RC4, hash.get(), 0, key.receive()))
        return 0;

    std::memmove(output, encrypted, length);
    DWORD decrypted_length = static_cast<DWORD>(length);
    if (!CryptDecrypt(key.get(), 0, TRUE, 0, output, &decrypted_length)) {
        SecureZeroMemory(output, length);
        return 0;
    }
    *output_length = decrypted_length;
    return 1;
}

int gg_read_signed_ini(const uint8_t *file, size_t file_length, const char *expected_name,
                       const uint8_t *public_blob, size_t public_blob_length, uint8_t *output,
                       size_t output_capacity, size_t *output_length) {
    gg_footer footer;

    if (!output_length)
        return 0;
    *output_length = 0;
    if (!gg_parse_footer(file, file_length, &footer) ||
        !gg_verify_signature(file, file_length, expected_name, public_blob, public_blob_length))
        return 0;
    return gg_decrypt_ini_payload(file, footer.payload_length, output, output_capacity,
                                  output_length);
}
