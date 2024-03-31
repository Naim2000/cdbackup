#include <stdio.h>
#include <sys/param.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "config.h"

typedef int (*ProgressCallback)(size_t now, size_t total, void* userp);
typedef ssize_t (*ReadCallback)(void* buffer, size_t len, void* userp);

#ifndef FS_CHUNK
#define FS_CHUNK 0x4000
#endif

int NAND_GetFileSize(const char* filepath, size_t*);
int FAT_GetFileSize(const char* filepath, size_t*);
int NAND_Read(const char* filepath, void* buffer, size_t filesize, ProgressCallback cb, void* userp);
int FAT_Read(const char* filepath, void* buffer, size_t filesize, ProgressCallback cb, void* userp);
int NAND_Write(const char* filepath, const void* buffer, size_t filesize, ProgressCallback cb, void* userp);
int FAT_Write(const char* filepath, const void* buffer, size_t filesize, ProgressCallback cb, void* userp);
int progressbar(size_t read, size_t filesize, void*);
int NANDBackupFile(const char*, const char*);
int NANDRestoreFile(const char*, const char*);
