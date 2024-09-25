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

#define SARC_DATA_ALIGN 128
#define SARC_NAME_ALIGN 4

#define SARC_DUMMY_NAME "DMY" // sizeof must be SARC_NAME_ALIGN

typedef struct __attribute((packed)) {
    u32 magic; // Compare to SARC_MAGIC
    u16 headerSize; // Always 0x14
    u16 boMarker; // Byte Order Mark: compare to BOMARKER_BIG and BOMARKER_LITTLE
    u32 fileSize; // File size of whole binary.
    u32 dataStart; // Offset to the archive data.

    u16 versionNumber; // Usually 0x0100

    u16 _reserved;
} SarcFileHeader;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to SFAT_MAGIC
    u16 headerSize; // Usually 0xC

    u16 nodeCount;
    u32 hashKey; // Usually 0x65
} SfatHeader;

typedef struct __attribute((packed)) {
    u32 nameHash;

    u16 nameOffsetDiv4;
    u16 isNameOffsetAvaliable;

    u32 dataOffsetStart; // Relative to the SARC header's dataStart
    u32 dataOffsetEnd; // Relative to the SARC header's dataStart
} SfatNode;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to SFNT_MAGIC
    u16 headerSize;

    u16 _pad16;
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

    if (
        fileHeader->boMarker != BOMARKER_LITTLE &&
        fileHeader->boMarker != BOMARKER_BIG
    )
        panic("SARC byte order mark is invalid");

    SfatHeader* sfatHeader = (SfatHeader*)(sarcData + fileHeader->headerSize);
    if (sfatHeader->magic != SFAT_MAGIC)
        panic("SFAT header magic is nonmatching");

    SfntHeader* sfntHeader = (SfntHeader*)(
        ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) +
        sfatHeader->nodeCount
    );
    if (sfntHeader->magic != SFNT_MAGIC)
        panic("SFNT header magic is nonmatching");

    if (fileHeader->boMarker != BOMARKER_BIG)
        return;

    fileHeader->headerSize = __builtin_bswap16(fileHeader->headerSize);
    fileHeader->dataStart = __builtin_bswap32(fileHeader->dataStart);

    sfatHeader->headerSize = __builtin_bswap16(sfatHeader->headerSize);
    sfatHeader->nodeCount = __builtin_bswap16(sfatHeader->nodeCount);
    sfatHeader->hashKey = __builtin_bswap32(sfatHeader->hashKey);

    // SFAT nodes
    for (u32 i = 0; i < sfatHeader->nodeCount; i++) {
        SfatNode* node =
            ((SfatNode*)((u8*)sfatHeader + sfatHeader->headerSize)) + i;

        node->nameHash = __builtin_bswap32(node->nameHash);

        node->nameOffsetDiv4 = __builtin_bswap16(node->nameOffsetDiv4);
        node->isNameOffsetAvaliable = __builtin_bswap16(node->isNameOffsetAvaliable);

        node->dataOffsetStart = __builtin_bswap32(node->dataOffsetStart);
        node->dataOffsetEnd = __builtin_bswap32(node->dataOffsetEnd);
    }

    sfntHeader->headerSize = __builtin_bswap16(sfntHeader->headerSize);

    fileHeader->boMarker = BOMARKER_LITTLE;
    return;
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

    SfntHeader* sfntHeader = (SfntHeader*)(
        ((SfatNode*)(
            sarcData + fileHeader->headerSize + sfatHeader->headerSize
        )) + sfatHeader->nodeCount
    );
    char* stringPtr = (char*)sfntHeader + sfntHeader->headerSize;

    if (node->isNameOffsetAvaliable != 0x0000)
        return stringPtr + (node->nameOffsetDiv4 * 4);

    // If name offset isn't avaliable, search the string pool for a string with
    // a matching hash
    for (u16 i = 0; i < sfatHeader->nodeCount; i++) {
        if (GetHash(stringPtr, strlen(stringPtr), sfatHeader->hashKey) == node->nameHash)
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

typedef struct {
    u8* ptr;
    u32 size;
} SarcBuildResult;

typedef struct {
    char* name;

    u8* data;
    u32 dataSize;

    int nil;
} SarcBuildFile;

SarcBuildResult SarcBuild(SarcBuildFile* files, u32 fileCount) {
    SarcBuildResult result;

    u32 initialSize =
        sizeof(SarcFileHeader) +
        sizeof(SfatHeader) +
        (sizeof(SfatNode) * fileCount);

    printf("Alloc initial buffer (size : %u) ..", initialSize);

    result.ptr = (u8*)malloc(initialSize);
    if (result.ptr == NULL)
        PANIC_MALLOC("initial build buf");

    LOG_OK;

    printf("Building file header & SFAT section ..");

    SarcFileHeader* fileHeader = (SarcFileHeader*)result.ptr;
    SfatHeader* sfatHeader = (SfatHeader*)(fileHeader + 1);

    fileHeader->magic = SARC_MAGIC;
    fileHeader->headerSize = sizeof(SarcFileHeader);
    fileHeader->boMarker = BOMARKER_LITTLE;
    fileHeader->versionNumber = 0x0100;
    fileHeader->_reserved = 0x0000;

    sfatHeader->magic = SFAT_MAGIC;
    sfatHeader->headerSize = sizeof(SfatHeader);
    sfatHeader->nodeCount = fileCount;
    sfatHeader->hashKey = 0x65;

    u32 nextNameOffset = 0;
    u32 nextDataOffset = 0;

    for (u32 i = 0; i < fileCount; i++) {
        SarcBuildFile* buildFile = files + i;
        SfatNode* node = (SfatNode*)(sfatHeader + 1) + i;

        if (buildFile->nil) {
            node->nameHash = GetHash(SARC_DUMMY_NAME, 3, sfatHeader->hashKey);

            node->nameOffsetDiv4 = nextNameOffset / 4;
            node->isNameOffsetAvaliable = 0x0100;

            nextNameOffset += SARC_NAME_ALIGN;

            node->dataOffsetStart = nextDataOffset;
            node->dataOffsetEnd = nextDataOffset + SARC_DATA_ALIGN;

            nextDataOffset += SARC_DATA_ALIGN;

            continue;
        }

        node->nameHash =
            GetHash(buildFile->name, strlen(buildFile->name), sfatHeader->hashKey);

        node->nameOffsetDiv4 = nextNameOffset / 4;
        node->isNameOffsetAvaliable = 0x0100;

        nextNameOffset += strlen(buildFile->name) + 1;
        if (i + 1 != fileCount)
            nextNameOffset = (nextNameOffset + SARC_NAME_ALIGN - 1) & ~(SARC_NAME_ALIGN - 1);

        node->dataOffsetStart = nextDataOffset;
        node->dataOffsetEnd = nextDataOffset + buildFile->dataSize;

        nextDataOffset += buildFile->dataSize;
        if (i + 1 != fileCount)
            nextDataOffset = (nextDataOffset + SARC_DATA_ALIGN - 1) & ~(SARC_DATA_ALIGN - 1);
    }

    LOG_OK;

    fileHeader->dataStart = (
        initialSize +
        sizeof(SfntHeader) + nextNameOffset +
        31
    ) & ~31;

    u32 newSize = fileHeader->dataStart + nextDataOffset;

    printf("Realloc buffer (size : %u) ..", newSize);

    result.ptr = realloc(result.ptr, newSize);
    if (result.ptr == NULL)
        PANIC_MALLOC("final build buf");

    result.size = newSize;

    fileHeader = (SarcFileHeader*)result.ptr;
    sfatHeader = (SfatHeader*)(fileHeader + 1);

    fileHeader->fileSize = newSize;

    LOG_OK;

    SfntHeader* sfntHeader = (SfntHeader*)(result.ptr + initialSize);

    sfntHeader->magic = SFNT_MAGIC;
    sfntHeader->headerSize = sizeof(SfntHeader);
    sfntHeader->_pad16 = 0x0000;

    char* nextString = (char*)(sfntHeader + 1);
    u8* nextData = (u8*)(result.ptr + fileHeader->dataStart);

    for (u32 i = 0; i < fileCount; i++) {
        SarcBuildFile* buildFile = files + i;

        if (buildFile->name) {
            strcpy(nextString, buildFile->name);
            nextString += ((strlen(buildFile->name) + 1) + SARC_NAME_ALIGN - 1) & ~(SARC_NAME_ALIGN - 1);
        }
        else {
            strcpy(nextString, "DMY");
            nextString += SARC_NAME_ALIGN;
        }

        if (buildFile->data) {
            memcpy(nextData, buildFile->data, buildFile->dataSize);
            nextData += (buildFile->dataSize + SARC_DATA_ALIGN - 1) & ~(SARC_DATA_ALIGN - 1);
        }
        else {
            memset(nextData, 0x00, SARC_DATA_ALIGN);
            nextData += SARC_DATA_ALIGN;
        }
    }

    return result;
}

#endif
