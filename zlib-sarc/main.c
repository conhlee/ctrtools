#include <stdio.h>
#include <stdlib.h>

#include "zlibProcess.h"
#include "sarcProcess.h"

#include "common.h"

#define CHECK_OUTPUT_GIVEN() { \
    if (args.outputPath == NULL) { \
        printf("Error: missing output path.\n\n"); \
        usage(0); \
    } \
}

ZlibResult ReadZLIBFromPath(char* zlibPath) {
    printf("Read & copy ZLIB binary ..");

    FILE* fpZlib = fopen(zlibPath, "rb");
    if (fpZlib == NULL)
        panic("The ZLIB binary could not be opened.");

    fseek(fpZlib, 0, SEEK_END);
    u64 compressedSize = ftell(fpZlib);
    rewind(fpZlib);

    u8* compressedBuf = (u8 *)malloc(compressedSize);
    if (compressedBuf == NULL) {
        fclose(fpZlib);

        PANIC_MALLOC("compressed buf");
    }

    u64 bytesCopied = fread(compressedBuf, 1, compressedSize, fpZlib);
    if (bytesCopied != compressedSize) {
        free(compressedBuf);
        fclose(fpZlib);

        panic("Buffer readin fail");
    }

    fclose(fpZlib);

    LOG_OK;

    ZlibResult decompression = decompressZlib(compressedBuf, compressedSize);

    free(compressedBuf);

    return decompression;
}

void usage(int title) {
    if (title) {
        printf("ZLIB-SARC Tool v2.0\n");
        printf("A tool for ZLIB-SARC (.zlib) archives.\n\n");
    }

    printf("Usage:\n");
    printf("    zlib-sarc [command] [options] <input>\n\n");

    printf("Commands:\n");
    printf("    extract   Extracts the contents of a ZLIB-SARC archive.\n");
    printf("    construct Constructs a ZLIB-SARC archive from individual files.\n");
    printf("    list      Lists the contents for a ZLIB-SARC archive.\n");
    printf("    raw       Export the raw SARC archive from a ZLIB-SARC archive.\n\n");

    printf("Options:\n");
    printf("    -o <path> Specifies the output path.\n");
    printf("    -l <path> Replicate the structure of the archive specified by this path.\n\n");

    printf("Examples:\n");
    printf("    zlib-sarc extract example.zlib -o ./output_directory\n");
    printf("    zlib-sarc construct ./example/anim/* ./example/blyt/* ./example/timg/* -o example.zlib\n");

    exit(1);
}

typedef struct {
    char* command;

    char* outputPath; // -o
    char* likePath; // -l

    u32 inputFileCount;
    char** inputFiles;
} Arguments;

int main(int argc, char* argv[]) {
    Arguments args;
    args.command = NULL;

    args.outputPath = NULL;
    args.likePath = NULL;
    
    args.inputFileCount = 0;
    args.inputFiles = NULL;

    if (argc < 3)
        usage(1);

    args.command = argv[1];

    int i = 2;
    while (i < argc) {
        if (argv[i][0] == '-') {
            if (strcasecmp(argv[i], "-o") == 0) {
                if (i + 1 < argc)
                    args.outputPath = argv[++i];
                else {
                    printf("Error: missing output path after -o.\n\n");
                    usage(0);
                }
            }
            else if (strcasecmp(argv[i], "-l") == 0) {
                if (i + 1 < argc)
                    args.likePath = argv[++i];
                else {
                    printf("Error: missing output path after -o.\n\n");
                    usage(0);
                }
            }
            else {
                printf("Error: unknown option (%s)\n\n", argv[i]);
                usage(0);
            }
        }
        else {
            args.inputFileCount++;
            args.inputFiles = realloc(args.inputFiles, args.inputFileCount * sizeof(char*));
            args.inputFiles[args.inputFileCount - 1] = argv[i];
        }

        i++;
    }

    if (args.inputFileCount <= 0) {
        printf("Error: missing input file(s).\n\n");
        usage(0);
    }

    if (strcasecmp(args.command, "extract") == 0) {
        CHECK_OUTPUT_GIVEN();

        printf("-- Extracting archive --\n\n");

        if (args.likePath)
            printf("Warning: a like path was passed but will not be used.\n");

        ZlibResult sarcBin = ReadZLIBFromPath(args.inputFiles[0]);

        SarcPreprocess(sarcBin.ptr);

        u16 nodeCount = SarcGetNodeCount(sarcBin.ptr);
        for (u16 i = 0; i < nodeCount; i++) {
            FindResult result = SarcGetFileFromIndex(sarcBin.ptr, i);

            char* name = SarcGetNameFromIndex(sarcBin.ptr, i);
            char nbuf[1024];

            if (!result.ptr)
                panic("A file could not be found.");
            if (!name)
                panic("A file's name could not be found.");

            printf("Writing file no. %u (%s) ..", i+1, name);

            int truncateAt = getFilename(name) - name;
            u32 outDirLen = strlen(args.outputPath);

            memcpy(nbuf, args.outputPath, outDirLen);
            nbuf[outDirLen] = PATH_SEPARATOR_C;
            memcpy(nbuf + outDirLen + 1, name, truncateAt);
            nbuf[outDirLen + 1 + truncateAt] = '\0';

            createDirectoryTree(nbuf);

            memcpy(nbuf + outDirLen + 1, name, strlen(name) + 1);

            FILE* fpOut = fopen(nbuf, "wb");
            if (fpOut == NULL)
                panic("The output binary could not be opened.");

            u64 bytesWritten = fwrite(result.ptr, 1, result.size, fpOut);
            if (bytesWritten != result.size) {
                fclose(fpOut);

                panic("The output binary could not be written to.");
            }

            fclose(fpOut);

            LOG_OK;
        }

        free(sarcBin.ptr);
    }
    else if (strcasecmp(args.command, "construct") == 0) {
        CHECK_OUTPUT_GIVEN();

        printf("-- Constructing archive --\n\n");

        SarcBuildFile* files = NULL;
        u32 fileCount = 0;

        if (args.likePath) {
            ZlibResult likeSarc = ReadZLIBFromPath(args.likePath);
            SarcPreprocess(likeSarc.ptr);

            fileCount = SarcGetNodeCount(likeSarc.ptr);

            printf("Construct matching build files:\n");

            // Array to track used input files
            int usedInputFiles[args.inputFileCount];
            memset(usedInputFiles, 0, sizeof(usedInputFiles));

            files = (SarcBuildFile*)malloc(sizeof(SarcBuildFile) * fileCount);
            if (!files)
                PANIC_MALLOC("build files");

            // Match files
            for (u32 a = 0; a < fileCount; a++) {
                char* sarcFileName = SarcGetNameFromIndex(likeSarc.ptr, a);
                SarcBuildFile* file = files + a;

                file->data = NULL;
                file->dataSize = 0;
                file->name = NULL;
                file->nil = 1;

                // Search for matching input files
                for (u32 b = 0; b < args.inputFileCount; b++) {
                    char bPath[512];
                    OSPathToSarcPath(args.inputFiles[b], bPath);

                    if (strcmp(sarcFileName, bPath) == 0) {
                        file->name = strdup(sarcFileName);
                        printf("Match found (%03u. %s), copying..\n", a + 1, file->name);

                        FILE* fpBin = fopen(args.inputFiles[b], "rb");
                        if (!fpBin)
                            panic("The file could not be opened.");

                        fseek(fpBin, 0, SEEK_END);
                        file->dataSize = ftell(fpBin);
                        rewind(fpBin);

                        file->data = (u8*)malloc(file->dataSize);
                        if (!file->data) {
                            fclose(fpBin);
                            PANIC_MALLOC("file buf");
                        }

                        u64 bytesCopied = fread(file->data, 1, file->dataSize, fpBin);
                        if (bytesCopied != file->dataSize) {
                            free(file->data);
                            fclose(fpBin);
                            panic("Buffer reading failed");
                        }

                        fclose(fpBin);
                        file->nil = 0;
                        usedInputFiles[b] = 1;

                        LOG_OK;
                        break;
                    }
                }

                if (!file->data)
                    printf("Match not found for file no. %u (%s).\n", a + 1, sarcFileName);
            }

            // Process additive files
            for (u32 b = 0; b < args.inputFileCount; b++) {
                if (!usedInputFiles[b] && !strchr(args.inputFiles[b], '*')) {
                    files = realloc(files, sizeof(SarcBuildFile) * (++fileCount));
                    if (!files)
                        PANIC_MALLOC("realloc build files");

                    SarcBuildFile* file = files + fileCount - 1;

                    char bPath[512];
                    OSPathToSarcPath(args.inputFiles[b], bPath);
                    file->name = strdup(bPath);

                    printf("Additive file found (%s), copying..", file->name);

                    FILE* fpBin = fopen(args.inputFiles[b], "rb");
                    if (!fpBin)
                        panic("The file could not be opened.");

                    fseek(fpBin, 0, SEEK_END);
                    file->dataSize = ftell(fpBin);
                    rewind(fpBin);

                    file->data = (u8*)malloc(file->dataSize);
                    if (!file->data) {
                        fclose(fpBin);
                        PANIC_MALLOC("file buf");
                    }

                    u64 bytesCopied = fread(file->data, 1, file->dataSize, fpBin);
                    if (bytesCopied != file->dataSize) {
                        free(file->data);
                        fclose(fpBin);
                        panic("Buffer reading failed");
                    }

                    fclose(fpBin);
                    LOG_OK;
                }
            }

        }
        else {
            printf("Construct build files: \n");

            fileCount = args.inputFileCount;

            files = (SarcBuildFile*)malloc(sizeof(SarcBuildFile) * fileCount);
            if (files == NULL)
                PANIC_MALLOC("build files");

            for (u32 j = 0; j < fileCount; j++) {
                SarcBuildFile* file = files + j;

                file->name = (char*)malloc(512);
                OSPathToSarcPath(args.inputFiles[j], file->name);

                printf("Read & copy file no. %u (%s) ..", j + 1, file->name);

                FILE* fpBin = fopen(args.inputFiles[j], "rb");
                if (fpBin == NULL)
                    panic("The file could not be opened.");

                fseek(fpBin, 0, SEEK_END);
                file->dataSize = ftell(fpBin);
                rewind(fpBin);

                file->data = (u8 *)malloc(file->dataSize);
                if (file->data == NULL) {
                    fclose(fpBin);

                    PANIC_MALLOC("file buf");
                }

                u64 bytesCopied = fread(file->data, 1, file->dataSize, fpBin);
                if (bytesCopied != file->dataSize) {
                    free(file->data);
                    fclose(fpBin);

                    panic("Buffer readin fail");
                }

                fclose(fpBin);

                LOG_OK;
            }
        }

        printf("\n");

        SarcBuildResult result = SarcBuild(files, fileCount);

        ZlibResult zlibBin = compressData(result.ptr, result.size);

        printf("Writing file data ..");

        FILE* fpOut = fopen(args.outputPath, "wb");
        if (fpOut == NULL)
            panic("Failed to open ZLIB out");

        if (fwrite(zlibBin.ptr, 1, zlibBin.size, fpOut) != zlibBin.size)
            panic("ZLIB write failed");
        
        fclose(fpOut);

        // Free build files
        for (u32 j = 0; j < args.inputFileCount; j++) {
            SarcBuildFile* file = files + j;

            if (file->name)
                free(file->name);
            if (file->data)
                free(file->data);
        }
        free(files);

        free(zlibBin.ptr);

        LOG_OK;
    }
    else if (strcasecmp(args.command, "list") == 0) {
        printf("-- Listing archive --\n\n");

        ZlibResult sarcBin = ReadZLIBFromPath(args.inputFiles[0]);

        SarcPreprocess(sarcBin.ptr);

        char prevPath[256] = "";  
        int prevDepth = 0;

        u16 nodeCount = SarcGetNodeCount(sarcBin.ptr);
        for (u16 i = 0; i < nodeCount; i++) {
            char* name = SarcGetNameFromIndex(sarcBin.ptr, i);
            FindResult file = SarcGetFileFromIndex(sarcBin.ptr, i);
            
            if (!name)
                panic("A file's name could not be found.");

            printf("%03u. %s (size: %u)\n", i+1, name, file.size);
        }

        free(sarcBin.ptr);
    }
    else if (strcasecmp(args.command, "raw") == 0) {
        CHECK_OUTPUT_GIVEN();

        printf("-- Exporting archive --\n\n");

        ZlibResult sarcBin = ReadZLIBFromPath(args.inputFiles[0]);

        printf("Writing file data ..");

        FILE* fpOut = fopen(args.outputPath, "wb");
        if (fpOut == NULL)
            panic("Failed to open SARC out");

        if (fwrite(sarcBin.ptr, 1, sarcBin.size, fpOut) != sarcBin.size)
            panic("SARC write failed");
        
        fclose(fpOut);

        free(sarcBin.ptr);
    }
    else {
        printf("Error: unknown command (%s)\n\n", args.command);
        usage(0);
    }

    printf("\nFinished! Exiting ..\n");

    free(args.inputFiles);

    return 0;
}
