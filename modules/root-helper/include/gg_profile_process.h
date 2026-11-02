#ifndef GG_PROFILE_PROCESS_H
#define GG_PROFILE_PROCESS_H

#include "gg_contract.h"
#include <stdint.h>

/* Read the loaded parent image without modifying it. Unknown layouts are rejected. */
gg_detection gg_detect_process_sdk(uint32_t pid);

#endif
