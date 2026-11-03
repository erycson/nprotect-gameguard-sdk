#define WIN32_LEAN_AND_MEAN
#include "gg_contract.h"
#include <windows.h>
#include <wincrypt.h>

static unsigned char ascii_lower(unsigned char c) {
    return c >= 'A' && c <= 'Z' ? (unsigned char)(c + ('a' - 'A')) : c;
}

int gg_verify_signature(const uint8_t *data, size_t length, const char *expected_name,
                        const uint8_t *public_blob, size_t public_blob_length) {
    gg_footer f;
    size_t i;
    HCRYPTPROV provider = 0;
    HCRYPTKEY key = 0;
    HCRYPTHASH hash = 0;
    int ok = 0;
    if (!expected_name || !public_blob || public_blob_length > MAXDWORD ||
        !gg_parse_footer(data, length, &f) || f.signed_length > MAXDWORD)
        return 0;
    for (i = 0; i + 1 < f.name_length; ++i) {
        if (!expected_name[i] ||
            ascii_lower((unsigned char)expected_name[i]) != ascii_lower(data[f.name_offset + i]))
            return 0;
    }
    if (expected_name[i])
        return 0;
    if (!CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        goto done;
    if (!CryptImportKey(provider, public_blob, (DWORD)public_blob_length, 0, 0, &key))
        goto done;
    if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash))
        goto done;
    if (!CryptHashData(hash, data, (DWORD)f.signed_length, 0))
        goto done;
    ok = CryptVerifySignatureA(hash, data + f.signature_offset, (DWORD)f.signature_length, key,
                               NULL, 0)
             ? 1
             : 0;
done:
    if (hash)
        CryptDestroyHash(hash);
    if (key)
        CryptDestroyKey(key);
    if (provider)
        CryptReleaseContext(provider, 0);
    return ok;
}
