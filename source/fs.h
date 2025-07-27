#include <stdio.h>
#include <sys/param.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "config.h"

#ifndef FS_CHUNK
#define FS_CHUNK 0x4000
#endif

int NAND_GetFileSize(const char* filepath, size_t*);
int NAND_Read(const char* filepath, void* buffer, size_t filesize);
int NAND_Write(const char* filepath, const void* buffer, size_t filesize);

int FAT_GetFileSize(const char* filepath, size_t*);
int FAT_Read(const char* filepath, void* buffer, size_t filesize);
int FAT_Write(const char* filepath, const void* buffer, size_t filesize);

//

int NANDBackupFile(const char*, const char*);
int NANDRestoreFile(const char*, const char*);
