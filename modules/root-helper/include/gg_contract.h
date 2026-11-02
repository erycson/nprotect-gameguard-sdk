#ifndef GG_CONTRACT_H
#define GG_CONTRACT_H

#include <stddef.h>
#include <stdint.h>

/* Profiles describe the SDK object layout and authentication message. */
typedef enum {
    GG_PROFILE_UNKNOWN = 0,
    GG_PROFILE_CSAUTH2_LEGACY = 1,
    GG_PROFILE_CSAUTH2_MODERN = 2,
    GG_PROFILE_CSAUTH1_LEGACY = 3
} gg_profile;

typedef struct {
    gg_profile profile;
    unsigned matches;
} gg_detection;

/* The result is unknown unless exactly one supported wrapper is found. */
gg_detection gg_detect_sdk(const uint8_t *text, size_t length);

typedef struct {
    size_t payload_length;
    size_t signed_length;
    size_t name_offset;
    size_t name_length; /* Includes the terminating NUL. */
    size_t signature_offset;
    size_t signature_length;
} gg_footer;

/* Structural parsing only; callers MUST separately verify the signature. */
int gg_parse_footer(const uint8_t *data, size_t length, gg_footer *out);
uint32_t gg_crc32(const uint8_t *data, size_t length);

/* Windows CryptoAPI PUBLICKEYBLOB; no private key is required or stored. */
int gg_verify_signature(const uint8_t *data, size_t length, const char *expected_name,
                        const uint8_t *public_blob, size_t public_blob_length);

#endif
