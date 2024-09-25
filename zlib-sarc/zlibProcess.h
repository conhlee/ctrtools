#ifndef ZLIBPROCESS_H
#define ZLIBPROCESS_H

#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#include "common.h"

typedef struct {
    u8* ptr;
    u32 size;
} ZlibResult;

ZlibResult decompressZlib(u8* zlibBinary, u32 binSize) {
    ZlibResult result;
    result.size = __builtin_bswap32(*(u32*)zlibBinary);

    printf("Alloc buffer (size : %u) ..", result.size);

    result.ptr = (u8*)malloc(result.size);
    if (result.ptr == NULL)
        PANIC_MALLOC("decompressed buf");

    LOG_OK;

    ///////////////////////////////////////

    printf("Init ZLIB ..");

    z_stream sInflate;
    sInflate.zalloc = Z_NULL;
    sInflate.zfree = Z_NULL;
    sInflate.opaque = Z_NULL;

    sInflate.avail_in = binSize - sizeof(u32);
    sInflate.next_in = zlibBinary + sizeof(u32);
    sInflate.avail_out = result.size;
    sInflate.next_out = result.ptr;

    LOG_OK;

    ///////////////////////////////////////

    printf("Decompressing ..");

    if (inflateInit(&sInflate) != Z_OK)
        panic("Inflate init failed");
    if (inflate(&sInflate, Z_NO_FLUSH) != Z_STREAM_END)
        panic("Inflate fail");
    inflateEnd(&sInflate);

    LOG_OK;

    return result;
}

ZlibResult compressData(u8* data, u32 dataSize) {
    ZlibResult result;

    u64 compressedMaxSize = compressBound(dataSize);

    printf("Alloc buffer (size : %lu) ..", compressedMaxSize + 4);

    result.ptr = (u8 *)malloc(compressedMaxSize + 4);
    if (!result.ptr)
        PANIC_MALLOC("compressed buf");

    LOG_OK;

    *(u32*)result.ptr = __builtin_bswap32(dataSize);

    z_stream sDeflate;
    sDeflate.zalloc = Z_NULL;
    sDeflate.zfree = Z_NULL;
    sDeflate.opaque = Z_NULL;

    sDeflate.avail_in = dataSize;
    sDeflate.next_in = data;
    sDeflate.avail_out = compressedMaxSize;
    sDeflate.next_out = result.ptr + 4;

    printf("Compressing ..");

    if (deflateInit(&sDeflate, Z_BEST_COMPRESSION) != Z_OK)
        panic("Deflate init failed");
    if (deflate(&sDeflate, Z_FINISH) != Z_STREAM_END)
        panic("Deflate fail");

    deflateEnd(&sDeflate);

    LOG_OK;

    result.size = sDeflate.total_out + 4;

    return result;
}

#endif
