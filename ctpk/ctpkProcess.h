#ifndef CTPKPROCESS_H
#define CTPKPROCESS_H

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "imageProcess.h"

#include "common.h"

#include <utime.h>
#include <time.h>

#define CTPK_MAGIC 0x4B505443 // "CTPK"

typedef struct {
    u32 magic; // Compare to CTPK_MAGIC
    u16 version;

    u16 textureCount;

    u32 textureSectionOffset; // Offset to the texture section.
    u32 textureSectionSize; // Size of the texture section.

    u32 hashSectionOffset; // Offset to the hash section.

    u32 textureInfoSection;

    u64 pad64;
} CtpkFileHeader;

typedef struct {
    u32 pathOffset; // Offset to file path

    u32 dataSize; // Size of texture data.
    u32 dataOffset; // Offset to texture data (relative to texture data block offset)
    u32 dataFormat; // Texture data format.

    u16 width; // Texture width
    u16 height; // Texture height

    u8 mipLevel; // Mipmap level
    u8 type; // 0: Cube Map, 1: 1D, 2: 2D

    u16 cubeDir;

    u32 bitmapSizeOffset; // Relative to to this block

    u32 srcTimestamp; // Unix timestamp of source texture.
} TextureEntry;

typedef struct {
    u32 pathHash; // CRC32 hash of the file path.

    u32 index;
} HashBlockEntry;

typedef struct {
    u8 textureFormat;
    u8 mipLevel;
    u8 compressed;
    u8 compressionMethod;
} TextureContextEntry;

/*
    Texture Formats:
        RGBA8888 = 0x00
        RGB888 = 0x01
        RGBA5551 = 0x02
        RGB565 = 0x03
        RGBA4444 = 0x04
        LA88 = 0x05
        HL8 = 0x06
        L8 = 0x07
        A8 = 0x08
        LA44 = 0x09
        L4 = 0x0A
        A4 = 0x0B
        ETC1 = 0x0C
        ETC1A4 = 0x0D
*/

void I_CtpkConvertTexture(u32** bufferOut, u32* sizeOut, const TextureEntry* textureInfoEntry, const CtpkFileHeader* fileHeader) {
	const u32* dataIn = (const u32*)(
	    (u8*)fileHeader + fileHeader->textureSectionOffset +
        textureInfoEntry->dataOffset
	);

	ImageProcessFunction function;

	switch (textureInfoEntry->dataFormat) {
    	case 0x0C:
            function = ProcessETC1;
            break;
	    case 0x0D:
			function = ProcessETC1A4;
			break;

	    default:
			panic("Texture format not implemented");
	}

	function(
	    bufferOut, sizeOut, dataIn,
		textureInfoEntry->width, textureInfoEntry->height
	);
}

void CtpkLogTextureNames(const u8* ctpkData) {
    CtpkFileHeader* fileHeader = (CtpkFileHeader*)ctpkData;

    if (fileHeader->magic != CTPK_MAGIC)
        panic("CTPK header magic is nonmatching");

    printf("Textures: \n");

    for (u32 i = 0; i < fileHeader->textureCount; i++) {
        TextureEntry* textureInfoEntry =
            (TextureEntry*)(fileHeader + 1) + i;

        printf(INDENT_SPACE "%d. %s\n", i + 1, ctpkData + textureInfoEntry->pathOffset);
    }
}

u16 CtpkGetTextureCount(const u8* ctpkData) {
    CtpkFileHeader* fileHeader = (CtpkFileHeader*)ctpkData;

    if (fileHeader->magic != CTPK_MAGIC)
        panic("CTPK header magic is nonmatching");

    return fileHeader->textureCount;
}

TextureEntry* CtpkGetTextureFromIndex(const u8* ctpkData, u16 textureIndex) {
    CtpkFileHeader* fileHeader = (CtpkFileHeader*)ctpkData;

    if (fileHeader->magic != CTPK_MAGIC)
        panic("CTPK header magic is nonmatching");

    if (textureIndex >= fileHeader->textureCount)
        return NULL;

    return (TextureEntry*)(fileHeader + 1) + textureIndex;
}

char* CtpkGetPathFromTextureIndex(const u8* ctpkData, u16 textureIndex) {
    CtpkFileHeader* fileHeader = (CtpkFileHeader*)ctpkData;

    if (fileHeader->magic != CTPK_MAGIC)
        panic("CTPK header magic is nonmatching");

    if (textureIndex >= fileHeader->textureCount)
        return NULL;

    TextureEntry* textureInfoEntry =
        (TextureEntry*)(fileHeader + 1) + textureIndex;

    return (char*)(ctpkData + textureInfoEntry->pathOffset);
}

TextureEntry* CtpkFindTextureFromPath(const u8* ctpkData, char* path) {
    CtpkFileHeader* fileHeader = (CtpkFileHeader*)ctpkData;

    if (fileHeader->magic != CTPK_MAGIC)
        panic("CTPK header magic is nonmatching");

    for (u32 i = 0; i < fileHeader->textureCount; i++) {
        TextureEntry* textureInfoEntry =
            (TextureEntry*)(fileHeader + 1) + i;

        if (strcmp(path, (char*)ctpkData + textureInfoEntry->pathOffset) == 0)
            return textureInfoEntry;
    }

    return NULL;
}

void CtpkExportTexture(const u8* ctpkData, TextureEntry* entry) {
    CtpkFileHeader* fileHeader = (CtpkFileHeader*)ctpkData;

    if (fileHeader->magic != CTPK_MAGIC)
        panic("CTPK header magic is nonmatching");

    u32 bufferSize;
    u32* buffer;

    I_CtpkConvertTexture(&buffer, &bufferSize, entry, fileHeader);

    char* filename = getFilename((char*)ctpkData + entry->pathOffset);

    if (stbi_write_tga(
        filename,
        entry->width, entry->height,
        4, buffer
    ) == 0)
        panic("Image write failed");

    setFileTimestamp(filename, entry->srcTimestamp);

    free(buffer);
}

#endif
