#ifndef GG_SYNC_H
#define GG_SYNC_H

#include "gg_startup.h"
#include <stdint.h>

/* Signal the inherited SDK event only after all required offline checks pass. */
int gg_signal_ready(const gg_startup_parameters *startup, uint32_t wait_ms);

#endif
