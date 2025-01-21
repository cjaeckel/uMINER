/*
 * SHA256d implementation based on Blockstream Jade (https://github.com/Blockstream/Jade)
 */
#ifndef SHA256plus_H_
#define SHA256plus_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SHA256 Mining context:
 */
struct SHA256mining {
public:
/* Calculate midstate */
IRAM_ATTR void InitBlockHeader(uint8_t* dataIn);

IRAM_ATTR bool SHA256d(uint8_t* dataIn, uint8_t* doubleHash);

private:
  uint8_t buffer[64];
  uint32_t digest[8];
};


void ByteReverseWords(uint32_t* out, const uint32_t* in, uint32_t byteCount);

#endif /* SHA256plus_H_ */