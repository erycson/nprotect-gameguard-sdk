#ifndef GG_LAUNCH_H
#define GG_LAUNCH_H

#include <stddef.h>
#include <stdint.h>

/*
 * Decodes the launch frame with escaped hexadecimal bytes observed in the v584 SDK.
 * The decoded bytes may contain session data; callers must not log them.
 * Returns 1 on success and 0 for an invalid or unsupported frame.
 */
int gg_decode_launch_frame(const char *hex, uint8_t *output, size_t output_capacity,
                           size_t *output_length);

#endif
