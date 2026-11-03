#include "gg_monitor.h"
#include "gg_contract.h"

int gg_validate_monitor(const uint8_t *file, size_t length, uint32_t expected_crc,
                        const uint8_t *public_blob, size_t public_blob_length) {
    gg_footer footer;
    if (!gg_parse_footer(file, length, &footer) || footer.payload_length < 64 || file[0] != 'M' ||
        file[1] != 'Z')
        return 0;
    return gg_crc32(file, length) == expected_crc &&
           gg_verify_signature(file, length, "GameMon.des", public_blob, public_blob_length);
}
