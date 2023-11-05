#include <ogc/isfs.h>
#include <fat.h>

typedef int (*RWCallback)(size_t read, size_t filesize);

#define MAXIMUM(max, size) ( ( size > max ) ? max : size )
#define FS_CHUNK 1048576

int FS_Read(const char* filepath, unsigned char** buffer, size_t* filesize, RWCallback cb);
int FS_Write(const char* filepath, unsigned char* buffer, size_t filesize, RWCallback cb);
int FAT_Read(const char* filepath, unsigned char** buffer, size_t* filesize, RWCallback cb);
int FAT_Write(const char* filepath, unsigned char* buffer, size_t filesize, RWCallback cb);
int progressbar(size_t read, size_t filesize);
