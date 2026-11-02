#ifndef GG_FILES_H
#define GG_FILES_H

#include "gg_startup.h"
#include <stddef.h>
#include <stdint.h>

/* Read and verify the local configuration and monitor without modifying them. */
int gg_validate_local_files(const gg_startup_parameters *startup, const uint8_t *public_key,
                            size_t public_key_length, uint32_t expected_monitor_crc,
                            uint32_t crc_values[4]);

#endif
