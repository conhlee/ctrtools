#include "etc1.hpp"

#include "rg_etc1.h"

void unpackETC1Block(void* etc1Block, unsigned int* dstPixels, int preserveAlpha) {
    rg_etc1::unpack_etc1_block(etc1Block, dstPixels, preserveAlpha != 0);
}
