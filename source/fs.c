#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "fs.h"

extern void* memalign(size_t, size_t);

[[gnu::aligned(0x20)]] static fstats stats;
[[gnu::aligned(0x20)]] static unsigned char tmp_buf[FS_CHUNK];

int progressbar(size_t read, size_t total) {
	printf("\r[");
	for (size_t i = 0; i < total; i += FS_CHUNK) {
		if (i < read)
			putchar('=');
		else
			putchar(' ');
	}
	printf("] %u / %u bytes (%.2f%%) ", read, total, (read / (double)total) * 100);

	return 0;
}

int FS_Read(const char* filepath, unsigned char** buffer, size_t* filesize, RWCallback callback) {
	int ret = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (ret < 0)
		return ret;

	int fd = ret;

	ret = ISFS_GetFileStats(fd, &stats);
	if (ret < 0) {
		ISFS_Close(fd);
		return ret;
	}
	*filesize = stats.file_length;

	*buffer = memalign(0x20, *filesize);
	if (!*buffer) {
		ISFS_Close(fd);
		return -ENOMEM;
	}

	size_t read = 0;
	while (read < *filesize) {
		ret = ISFS_Read(fd, tmp_buf, FS_CHUNK);
		if (ret <= 0)
			break;

		memcpy((*buffer) + read, tmp_buf, ret);
		read += ret;
		if (callback) callback(read, *filesize);
	}

	ISFS_Close(fd);

	if (read == *filesize)
		return 0;

	free(*buffer);
	if (ret < 0)
		return ret;
	else
		return -EIO;
}

int FAT_Read(const char* filepath, unsigned char** buffer, size_t* filesize, RWCallback callback) {
	FILE* fp = fopen(filepath, "rb");
	if (!fp)
        return -errno;

	fseek(fp, 0, SEEK_END);
	*filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(!*filesize)
        return -105;

	*buffer = memalign(0x20, *filesize);
	if(!*buffer) {
		fclose(fp);
		return -ENOMEM;
	};

	size_t read = 0;
	while (read < *filesize) {
		size_t _read = fread(tmp_buf, 1, FS_CHUNK, fp);
		if (!_read)
			break;

		memcpy((*buffer) + read, tmp_buf, _read);
		read += _read;
		if (callback) callback(read, *filesize);
	}
	fclose(fp);

	if (read == *filesize)
		return 0;

	free(*buffer);
	if (errno) //?
		return -errno;
	else
		return -EIO;
}

int FS_Write(const char* filepath, unsigned char* buffer, size_t filesize, RWCallback callback) {
	int ret = ISFS_Open(filepath, ISFS_OPEN_WRITE);
	if (ret < 0)
		return ret;

	int fd = ret;

	size_t wrote = 0;
	while (wrote < filesize) {
		size_t _count = MAXIMUM(FS_CHUNK, filesize - wrote);
		memcpy(tmp_buf, buffer + wrote, _count);
		ret = ISFS_Write(fd, tmp_buf, _count);
		if (ret <= 0)
			break;

		wrote += ret;
		if (callback) callback(wrote, filesize);
	}

	ISFS_Close(fd);
	if (wrote == filesize)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

int FAT_Write(const char* filepath, unsigned char* buffer, size_t filesize, RWCallback callback) {
	FILE *fp = fopen(filepath, "wb");
	if (!fp) return -errno;

	size_t wrote = 0;
	while (wrote < filesize) {
		size_t _count = MAXIMUM(FS_CHUNK, filesize - wrote);
		memcpy(tmp_buf, buffer + wrote, _count);
		size_t _wrote = fwrite(tmp_buf, 1, _count, fp);
		if (!_wrote)
			break;

		wrote += _wrote;
		if (callback) callback(wrote, filesize);
	}
	fclose(fp);

	if (wrote == filesize)
		return 0;
	else if (errno) //?
		return errno;
	else
		return -EIO;
}
