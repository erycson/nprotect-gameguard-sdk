#ifndef GG_MONITOR_H
#define GG_MONITOR_H

#include <stddef.h>
#include <stdint.h>

/* The expected CRC covers the complete signed file including its footer. */
int gg_validate_monitor(const uint8_t *file, size_t length, uint32_t expected_crc,
                        const uint8_t *public_blob, size_t public_blob_length);

#endif
