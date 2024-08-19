#ifndef IMAGEPROCESS_H
#define IMAGEPROCESS_H

#include "common.h"

void unpackETC1Block(void* etc1Block, unsigned int* dstPixels, int preserveAlpha);

typedef void (*ImageProcessFunction)(u32**, u32*, const u32*, u16, u16);

void ProcessETC1A4(u32** bufferOut, u32* sizeOut, const u32* dataIn, u16 width, u16 height) {
    u32 bufferSize = width * height * 4;
	u32* buffer = (u32*)malloc(bufferSize);

	u32 inOffset = 0;

	for (u32 xImage = 0; xImage < width; xImage += 8) {
		for (u32 yImage = 0; yImage < height; yImage += 8) {
			u64 data;
			u32 pixels[4 * 4];

			for (unsigned z = 0; z < 4; z++) {
				unsigned xStart = (z == 0 || z == 2 ? 0 : 4);
				unsigned yStart = (z == 0 || z == 1 ? 0 : 4);

				memcpy(&data, &dataIn[inOffset], sizeof(unsigned long long));
				inOffset += 2;

				for (u32 y = yImage + xStart; y < yImage + xStart + 4; y++)
					for (u32 x = xImage + yStart; x < xImage + yStart + 4; x++) {
						buffer[(x * height) + y] =
						    ((((data & 0xF) << 28) | ((data & 0xF) << 24)) & 0xFF000000);
						data >>= 4;
					}

				xStart = (z == 0 || z == 1 ? 0 : 4);
				yStart = (z == 0 || z == 2 ? 0 : 4);

				*(u64*)&data = __builtin_bswap64(*(u64*)&dataIn[inOffset]);
				inOffset += 2;

				unpackETC1Block(&data, pixels, FALSE);

				u32* lPixel = pixels;
				for (u32 x = xImage + xStart; x < xImage + xStart + 4; x++)
				for (u32 y = yImage + yStart; y < yImage + yStart + 4; y++)
					buffer[(x * height) + y] |= (*(lPixel++) & 0xFFFFFF);
			}
		}
	}

	*bufferOut = buffer;
	*sizeOut = bufferSize;
}

void ProcessETC1(u32** bufferOut, u32* sizeOut, const u32* dataIn, u16 width, u16 height) {
    u32 bufferSize = width * height * 4;
	u32* buffer = (u32*)malloc(bufferSize);

	u32 inOffset = 0;

	for (u32 xImage = 0; xImage < width; xImage += 8) {
		for (u32 yImage = 0; yImage < height; yImage += 8) {
			u64 data;
			u32 pixels[4 * 4];

			for (unsigned z = 0; z < 4; z++) {
				unsigned xStart = (z == 0 || z == 1 ? 0 : 4);
				unsigned yStart = (z == 0 || z == 2 ? 0 : 4);

				*(u64*)&data = __builtin_bswap64(*(u64*)&dataIn[inOffset]);
				inOffset += 2;

				unpackETC1Block(&data, pixels, FALSE);

				u32* lPixel = pixels;
				for (u32 x = xImage + xStart; x < xImage + xStart + 4; x++)
				for (u32 y = yImage + yStart; y < yImage + yStart + 4; y++)
						buffer[(x * height) + y] = *(lPixel++);
			}
		}
	}

	*bufferOut = buffer;
	*sizeOut = bufferSize;
}

#endif
