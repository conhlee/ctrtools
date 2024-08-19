#ifndef SARCPROCESS_H
#define SARCPROCESS_H

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "common.h"

#define SARC_MAGIC 0x43524153 // "SARC"
#define SFAT_MAGIC 0x54414653 // "SFAT"
#define SFNT_MAGIC 0x544E4653 // "SFNT"

#define BOMARKER_BIG 0xFFFE
#define BOMARKER_LITTLE 0xFEFF

typedef struct {
    u32 magic; // Compare to SARC_MAGIC
    u16 headerSize; // Always 0x14
    u16 boMarker; // Byte Order Mark: compare to BOMARKER_BIG and BOMARKER_LITTLE
    u32 fileSize; // File size of whole binary.
    u32 dataStart; // Offset to the archive data.

    u16 versionNumber; // Usually 0x0100

    u16 _reserved;
} SarcFileHeader;

typedef struct {
    u32 magic; // Compare to SFAT_MAGIC
    u16 headerSize; // Usually 0xC

    u16 nodeCount;
    u32 hashKey; // Usually 0x65
} SfatHeader;

typedef struct {
    u32 nameHash;

    u32 attributes;

    u32 dataOffsetStart; // Relative to the SARC header's dataStart
    u32 dataOffsetEnd; // Relative to the SARC header's dataStart
} SfatNode;

typedef struct {
    u32 magic; // Compare to SFNT_MAGIC
    u16 headerSize;

    u16 pad16;
} SfntHeader;

u32 GetHash(const char* name, u32 length, u32 key) {
    u32 result = 0;
	for (u32 i = 0; i < length; i++)
		result = name[i] + result * key;

	return result;
}

void SarcPreprocess(u8* sarcData) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;
    if (fileHeader->magic != SARC_MAGIC)
        panic("SARC header magic is nonmatching");

    if (fileHeader->boMarker != BOMARKER_BIG)
        return;

    fileHeader->headerSize = __builtin_bswap16(fileHeader->headerSize);
    fileHeader->dataStart = __builtin_bswap32(fileHeader->dataStart);

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);
    if (sfatHeader->magic != SFAT_MAGIC)
        panic("SFAT header magic is nonmatching");

    sfatHeader->headerSize = __builtin_bswap16(sfatHeader->headerSize);
    sfatHeader->nodeCount = __builtin_bswap16(sfatHeader->nodeCount);
    sfatHeader->hashKey = __builtin_bswap32(sfatHeader->hashKey);

    // SFAT nodes
    for (u32 i = 0; i < sfatHeader->nodeCount; i++) {
        SfatNode* node =
            ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) + i;

        node->nameHash = __builtin_bswap32(node->nameHash);

        node->attributes = __builtin_bswap32(node->attributes);

        node->dataOffsetStart = __builtin_bswap32(node->dataOffsetStart);
        node->dataOffsetEnd = __builtin_bswap32(node->dataOffsetEnd);
    }

    SfntHeader* sfntHeader = (SfntHeader*)(
        ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) +
        sfatHeader->nodeCount
    );
    if (sfntHeader->magic != SFNT_MAGIC)
        panic("SFNT header magic is nonmatching");

    sfntHeader->headerSize = __builtin_bswap16(sfntHeader->headerSize);

    fileHeader->boMarker = BOMARKER_LITTLE;
    return;
}

void SarcLogFilenames(const u8* sarcData) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);

    SfntHeader* sfntHeader = (SfntHeader*)(
        ((SfatNode*)(
            sarcData + fileHeader->headerSize + sfatHeader->headerSize
        )) + sfatHeader->nodeCount
    );

    printf("Files: \n");

    char* stringPtr = (char*)sfntHeader + sfntHeader->headerSize;
    for (u32 i = 0; i < sfatHeader->nodeCount; i++) {
        printf(INDENT_SPACE "%d. %s\n", i + 1, stringPtr);

        u32 length = strlen(stringPtr) + 1;
        stringPtr += (length + 3) & ~3;
    }
}

typedef struct {
    u8* ptr;
    u32 size;
} FindResult;

u16 SarcGetNodeCount(const u8* sarcData) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);

    return sfatHeader->nodeCount;
}

char* SarcGetNameFromHash(const u8* sarcData, u32 hash) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);

    SfntHeader* sfntHeader = (SfntHeader*)(
        ((SfatNode*)(
            sarcData + fileHeader->headerSize + sfatHeader->headerSize
        )) + sfatHeader->nodeCount
    );

    char* stringPtr = (char*)sfntHeader + sfntHeader->headerSize;
    for (u32 i = 0; i < sfatHeader->nodeCount; i++) {
        if (GetHash(stringPtr, strlen(stringPtr), sfatHeader->hashKey) == hash)
            return stringPtr;

        u32 length = strlen(stringPtr) + 1;
        stringPtr += (length + 3) & ~3;
    }

    return NULL;
}

char* SarcGetNameFromIndex(const u8* sarcData, u16 nodeIndex) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;

    const u8* dataStart = sarcData + fileHeader->dataStart;

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);

    if (nodeIndex >= sfatHeader->nodeCount)
        return NULL;

    SfatNode* node =
        ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) + nodeIndex;

    u32 searchHash = node->nameHash;

    SfntHeader* sfntHeader = (SfntHeader*)(
        ((SfatNode*)(
            sarcData + fileHeader->headerSize + sfatHeader->headerSize
        )) + sfatHeader->nodeCount
    );

    char* stringPtr = (char*)sfntHeader + sfntHeader->headerSize;
    for (u32 i = 0; i < sfatHeader->nodeCount; i++) {
        if (GetHash(stringPtr, strlen(stringPtr), sfatHeader->hashKey) == searchHash)
            return stringPtr;

        u32 length = strlen(stringPtr) + 1;
        stringPtr += (length + 3) & ~3;
    }

    return NULL;
}

FindResult SarcGetFileFromIndex(u8* sarcData, u16 nodeIndex) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;

    u8* dataStart = sarcData + fileHeader->dataStart;

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);

    FindResult result;
    result.ptr = NULL;
    result.size = 0;

    if (nodeIndex >= sfatHeader->nodeCount)
        return result;

    SfatNode* node =
        ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) + nodeIndex;

    result.ptr = dataStart + node->dataOffsetStart;
    result.size = node->dataOffsetEnd - node->dataOffsetStart;

    return result;
}

FindResult SarcFindFile(u8* sarcData, const char* name) {
    SarcFileHeader* fileHeader = (SarcFileHeader*)sarcData;

    u8* dataStart = sarcData + fileHeader->dataStart;

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);
    u32 nameHash = GetHash(name, strlen(name), sfatHeader->hashKey);

    FindResult result;
    result.ptr = NULL;
    result.size = 0;

    for (u32 i = 0; i < sfatHeader->nodeCount; i++) {
        SfatNode* node =
            ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) + i;

        if (node->nameHash == nameHash) {
            result.ptr = dataStart + node->dataOffsetStart;
            result.size = node->dataOffsetEnd - node->dataOffsetStart;
            break;
        }
    }

    return result;
}

#endif
