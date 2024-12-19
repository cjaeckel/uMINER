/*
 * SHA256d implementation based on Blockstream Jade (https://github.com/Blockstream/Jade)
 */
#ifndef SHA256plus_H_
#define SHA256plus_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct mining_SHA256_ctx {
    uint8_t buffer[64];
    uint32_t digest[8];
};

/* Calculate midstate */
IRAM_ATTR void SHA_midstate(mining_SHA256_ctx* midstate, uint8_t* dataIn);

IRAM_ATTR bool SHA256d(mining_SHA256_ctx* midstate, uint8_t* dataIn, uint8_t* doubleHash);

void ByteReverseWords(uint32_t* out, const uint32_t* in, uint32_t byteCount);

#endif /* SHA256plus_H_ */