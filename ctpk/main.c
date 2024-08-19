#include <stdio.h>
#include <stdlib.h>

#include "ctpkProcess.h"

#include "common.h"

void ExportTexture(u8* ctpkData, char* findPath) {
    TextureEntry* entry = CtpkFindTextureFromPath(ctpkData, findPath);

    if (!entry)
        panic("The texture was not found.");

    printf("Write to file ..");

    CtpkExportTexture(ctpkData, entry);

    LOG_OK;
}

void ExportAllTextures(u8* ctpkData) {
    u16 nodeCount = CtpkGetTextureCount(ctpkData);

    for (u16 i = 0; i < nodeCount; i++) {
        TextureEntry* entry = CtpkGetTextureFromIndex(ctpkData, i);
        char* name = CtpkGetPathFromTextureIndex(ctpkData, i);

        if (!entry)
            panic("A texture could not be found.");
        if (!name)
            panic("A texture's path could not be found.");

        printf("Writing texture no. %u ..", i+1);

        CtpkExportTexture(ctpkData, entry);

        LOG_OK;
    }
}

void usage() {
    printf("CTPK Tool v1.0\n");
    printf("A tool for extracting textures from CTPK texture archives.\n\n");

    printf("Usage: ctpkt <path_to_ctpk> [texture_to_extract]\n");
    printf("  <path_to_ctpk>         Path to the CTPK file.\n");
    printf("  [texture_to_extract]   (Optional) Path of the texture to extract.\n");
    printf("                         If omitted, a list of all textures will be displayed.\n");
    printf("                         Use 'ALL' to extract all files in the archive.\n\n");

    printf("Examples:\n");
    printf("  ctpkt ./sample.ctpk\n");
    printf("  ctpkt ./sample.ctpk path/to/texture\n");
    printf("  ctpkt ./sample.ctpk ALL\n");

    exit(1);
}

int main(int argc, char* argv[]) {
    FILE* fpCtpk;
    u8* ctpkBuf;
    u64 ctpkSize;

    char* ctpkPath;
    char* findPath;

    if (argc < 2)
        usage();

    ctpkPath = argv[1];
    findPath = argc > 2 ? argv[2] : NULL;

    printf("Read & copy CTPK binary ..");

    fpCtpk = fopen(ctpkPath, "rb");
    if (fpCtpk == NULL)
        panic("The CTPK binary could not be opened.");

    fseek(fpCtpk, 0, SEEK_END);
    ctpkSize = ftell(fpCtpk);
    rewind(fpCtpk);

    ctpkBuf = (u8 *)malloc(ctpkSize);
    if (ctpkBuf == NULL) {
        fclose(fpCtpk);

        panic("Mem alloc fail (ctpk buf)");
    }

    u64 bytesCopied = fread(ctpkBuf, 1, ctpkSize, fpCtpk);
    if (bytesCopied != ctpkSize) {
        free(ctpkBuf);
        fclose(fpCtpk);

        panic("Buffer readin fail");
    }

    fclose(fpCtpk);

    LOG_OK;

    ////////////////////////////////////////

    if (findPath) {
        if (strcmp(findPath, "ALL") == 0)
            ExportAllTextures(ctpkBuf);
        else
            ExportTexture(ctpkBuf, findPath);
    }
    else {
        CtpkLogTextureNames(ctpkBuf);

        printf("\n!> To export a texture, append the path as a second argument.\n");
        printf("   To export all textures, enter 'ALL' as the second argument.\n");
    }

    printf("\nFinished! Exiting ..\n");

    return 0;
}
