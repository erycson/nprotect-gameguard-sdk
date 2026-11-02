#ifndef GG_INI_H
#define GG_INI_H

#include <stddef.h>
#include <stdint.h>

/* Call only after verifying the signed INI with gg_verify_signature. */
int gg_decrypt_ini_payload(const uint8_t *encrypted, size_t length, uint8_t *output,
                           size_t output_capacity, size_t *output_length);

/* Verify the signed filename and payload before decrypting the INI. */
int gg_read_signed_ini(const uint8_t *file, size_t file_length, const char *expected_name,
                       const uint8_t *public_blob, size_t public_blob_length, uint8_t *output,
                       size_t output_capacity, size_t *output_length);

#endif
