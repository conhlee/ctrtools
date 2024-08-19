#include <stdio.h>
#include <stdlib.h>

#include "zlibProcess.h"
#include "sarcProcess.h"

#include "common.h"

void ExportFile(u8* sarcData, char* findPath) {
    FindResult findResult = SarcFindFile(sarcData, findPath);

    if (!findResult.ptr)
        panic("The file was not found.");

    printf("Write to file ..");

    FILE* fpOut = fopen(getFilename(findPath), "wb");
    if (fpOut == NULL)
        panic("The out binary could not be opened.");

    u64 bytesWritten = fwrite(findResult.ptr, 1, findResult.size, fpOut);
    if (bytesWritten != findResult.size) {
        fclose(fpOut);

        panic("File write fail");
    }

    fclose(fpOut);

    LOG_OK;
}

void ExportAllFiles(u8* sarcData) {
    u16 nodeCount = SarcGetNodeCount(sarcData);

    for (u16 i = 0; i < nodeCount; i++) {
        FindResult result = SarcGetFileFromIndex(sarcData, i);
        char* name = SarcGetNameFromIndex(sarcData, i);

        if (!result.ptr)
            panic("A file could not be found.");
        if (!name)
            panic("A file's name could not be found.");

        printf("Writing file no. %u ..", i+1);

        FILE* fpOut = fopen(getFilename(name), "wb");
        if (fpOut == NULL)
            panic("The out binary could not be opened.");

        u64 bytesWritten = fwrite(result.ptr, 1, result.size, fpOut);
        if (bytesWritten != result.size) {
            fclose(fpOut);

            panic("File write fail");
        }

        fclose(fpOut);

        LOG_OK;
    }
}

void usage() {
    printf("ZLIB-SARC Tool v1.0\n");
    printf("A tool for extracting files from ZLIB archives containing SARC files.\n\n");

    printf("Usage: zlib-sarc <path_to_zlib> [file_to_extract]\n");
    printf("  <path_to_zlib>      Path to the ZLIB archive file.\n");
    printf("  [file_to_extract]   (Optional) Path of the file to extract from the archive.\n");
    printf("                      If omitted, a list of all files in the archive will be displayed.\n");
    printf("                      Use 'ALL' to extract all files in the archive.\n\n");

    printf("Examples:\n");
    printf("  zlib-sarc ./sample.zlib\n");
    printf("  zlib-sarc ./sample.zlib path/to/file\n");
    printf("  zlib-sarc ./sample.zlib ALL\n");

    exit(1);
}

int main(int argc, char* argv[]) {
    FILE* fpZlib;
    u8* compressedBuf;
    u64 compressedSize;

    FILE* fpOut;
    DecompressResult decompression;

    char* zlibPath;
    char* findPath;

    FindResult findResult;

    if (argc < 2)
        usage();

    zlibPath = argv[1];
    findPath = argc > 2 ? argv[2] : NULL;

    printf("Read & copy ZLIB binary ..");

    fpZlib = fopen(zlibPath, "rb");
    if (fpZlib == NULL)
        panic("The ZLIB binary could not be opened.");

    fseek(fpZlib, 0, SEEK_END);
    compressedSize = ftell(fpZlib);
    rewind(fpZlib);

    compressedBuf = (u8 *)malloc(compressedSize);
    if (compressedBuf == NULL) {
        fclose(fpZlib);

        panic("Mem alloc fail (compressed buf)");
    }

    u64 bytesCopied = fread(compressedBuf, 1, compressedSize, fpZlib);
    if (bytesCopied != compressedSize) {
        free(compressedBuf);
        fclose(fpZlib);

        panic("Buffer readin fail");
    }

    fclose(fpZlib);

    LOG_OK;

    decompression = decompressZlib(compressedBuf, compressedSize);

    ////////////////////////////////////////

    SarcPreprocess(decompression.ptr);

    if (findPath) {
        if (strcmp(findPath, "ALL") == 0)
            ExportAllFiles(decompression.ptr);
        else
            ExportFile(decompression.ptr, findPath);
    }
    else {
        SarcLogFilenames(decompression.ptr);

        printf("\n!> To export a file, append the path as a second argument.\n");
        printf("   To export all files, enter 'ALL' as the second argument.\n");
    }

    printf("\nFinished! Exiting ..\n");

    return 0;
}
