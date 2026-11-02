#ifndef GG_PARENT_H
#define GG_PARENT_H

#include <stdint.h>

/* Only operates on this helper's actual parent; does not modify executable bytes. */
int gg_prepare_parent(uint32_t expected_pid);

#endif
