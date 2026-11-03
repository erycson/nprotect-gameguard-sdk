#include "gg_ini.h"
#include "gg_contract.h"

#include <limits.h>
#include <string.h>
#include <windows.h>
#include <wincrypt.h>

int gg_decrypt_ini_payload(const uint8_t *encrypted, size_t length, uint8_t *output,
                           size_t output_capacity, size_t *output_length) {
    static const uint8_t secret[] = "ectGameMon";
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    HCRYPTKEY key = 0;
    DWORD decrypted_length;
    int success = 0;

    if (!output_length)
        return 0;
    *output_length = 0;
    if (!encrypted || !output || !length || length > MAXDWORD || output_capacity < length)
        return 0;
    if (!CryptAcquireContextA(&provider, NULL, MS_DEF_PROV_A, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider, CALG_MD5, 0, 0, &hash) ||
        !CryptHashData(hash, secret, (DWORD)(sizeof secret - 1u), 0) ||
        !CryptDeriveKey(provider, CALG_RC4, hash, 0, &key))
        goto cleanup;
    memmove(output, encrypted, length);
    decrypted_length = (DWORD)length;
    if (!CryptDecrypt(key, 0, TRUE, 0, output, &decrypted_length)) {
        SecureZeroMemory(output, length);
        goto cleanup;
    }
    *output_length = decrypted_length;
    success = 1;
cleanup:
    if (key)
        CryptDestroyKey(key);
    if (hash)
        CryptDestroyHash(hash);
    if (provider)
        CryptReleaseContext(provider, 0);
    return success;
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
