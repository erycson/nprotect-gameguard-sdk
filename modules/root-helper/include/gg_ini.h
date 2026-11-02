#ifndef GG_INI_H
#define GG_INI_H

#include <stddef.h>
#include <stdint.h>

/* Call only after verifying the signed INI with gg_verify_signature. */
int gg_decrypt_ini_payload(const uint8_t *encrypted, size_t length, uint8_t *output,
                           size_t output_capacity, size_t *output_length);

#endif
