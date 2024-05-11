#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "fs.h"

__aligned(0x20)
static char fsBuffer[FS_CHUNK];

#define PROGRESSBAR_WIDTH 20
int progressbar(size_t read, size_t total, void* userp) {
	printf("\r[\x1b[42;1m");

	unsigned chunk = total / PROGRESSBAR_WIDTH;
	for (size_t i = 0; i < total; i += chunk)  {
		if (i > read)
			printf("\x1b[40m");

		putchar(' ');
	}

	printf("\x1b[40m] %u / %u bytes (%i%%) ", read, total, (read * 100 / total));
	if (read == total)
		putchar('\n');

	return 0;
}

int NAND_GetFileSize(const char* filepath, size_t* size) {
	__attribute__((aligned(0x20)))
	static fstats file_stats;

	if (size)
		*size = 0;

	int fd = ISFS_Open(filepath, 0);
	if (fd < 0)
		return fd;

	if (size) {
		int ret = ISFS_GetFileStats(fd, &file_stats);
		if (ret < 0)
			return ret;
		*size = file_stats.file_length;
	}
	ISFS_Close(fd);
	return 0;
}

int FAT_GetFileSize(const char* filepath, size_t* size) {
	static struct stat st = {};

	if (stat(filepath, &st) < 0) return -errno;

	if (size) *size = st.st_size;
	return 0;
}

int NAND_Read(const char* filepath, void* buffer, size_t filesize, ProgressCallback callback, void* userp) {
	if (!filesize || !buffer) return -EINVAL;

	int ret = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (ret < 0)
		return ret;

	int fd = ret;
	size_t read = 0;
	while (read < filesize) {
		ret = ISFS_Read(fd, fsBuffer, MIN(FS_CHUNK, filesize - read));
		if (ret <= 0)
			break;

		memcpy(buffer + read, fsBuffer, ret);
		read += ret;
		if (callback) callback(read, filesize, userp);
	}

	ISFS_Close(fd);

	if (read == filesize)
		return 0;

	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

int FAT_Read(const char* filepath, void* buffer, size_t filesize, ProgressCallback callback, void* userp) {
	FILE* fp = fopen(filepath, "rb");
	if (!fp)
        return -errno;

	size_t read = 0;
	while (read < filesize) {
		size_t _read = fread(buffer + read, 1, MIN(FS_CHUNK, filesize - read), fp);
		if (!_read)
			break;

		read += _read;
		if (callback) callback(read, filesize, userp);
	}
	fclose(fp);

	if (read == filesize)
		return 0;
	if (errno)
		return -errno;
	else
		return -EIO;
}

int NAND_Write(const char* filepath, const void* buffer, size_t filesize, ProgressCallback callback, void* userp) {
	if (!buffer != !filesize) return -EINVAL;
	
	int ret = ISFS_Open(filepath, ISFS_OPEN_WRITE);
	if (ret < 0)
		return ret;

	int fd = ret;
	size_t wrote = 0;
	while (wrote < filesize) {
		size_t len = MIN(FS_CHUNK, filesize - wrote);
		memcpy(fsBuffer, buffer + wrote, len);
		ret = ISFS_Write(fd, fsBuffer, len);
		if (ret <= 0)
			break;

		wrote += ret;
		if (callback) callback(wrote, filesize, userp);
	}

	ISFS_Close(fd);
	if (wrote == filesize)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

int FAT_Write(const char* filepath, const void* buffer, size_t filesize, ProgressCallback callback, void* userp) {
	if (!buffer || !filesize) return -EINVAL;
	
	FILE* fp = fopen(filepath, "wb");
	if (!fp)
		return -errno;

	size_t wrote = 0;
	while (wrote < filesize) {
		size_t _wrote = fwrite(buffer + wrote, 1, MIN(FS_CHUNK, filesize - wrote), fp);
		if (!_wrote)
			break;

		wrote += _wrote;
		if (callback) callback(wrote, filesize, userp);
	}
	fclose(fp);

	if (wrote == filesize)
		return 0;
	else if (errno) // tends to be set
		return -errno;
	else
		return -EIO;
}

int NANDBackupFile(const char* filepath, const char* dest) {
	int ret, fd;
	size_t fsize = 0;

	printf("%s ==> %s\n", filepath, dest);
	ret = NAND_GetFileSize(filepath, &fsize);
	
	if (ret < 0) {
		printf("%s: retreiving file size failed (%i)\n", filepath, ret);
		return ret;
	}

	progressbar(0, fsize, 0);

	fd = ret = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (ret < 0) {
		printf("%s: failed to open (%i)\n", filepath, ret);
		return ret;
	}

	FILE* fp = fopen(dest, "wb");
	if (!fp) {
		printf("%s: ", dest);
		perror("failed to open");
		return -errno;
	}

	size_t total = 0;

	while (total < fsize) {
		ret = ISFS_Read(fd, fsBuffer, FS_CHUNK);
		if (ret <= 0)
			break;

		if (!fwrite(fsBuffer, ret, 1, fp)) {
			ret = -errno;
			break;
		}
		
		total += ret;

		progressbar(total, fsize, 0);
	}
	ISFS_Close(fd);
	fclose(fp);
	
	putchar('\n');
	if (ret < 0) {
		printf("Error! (%i)\n", ret);
		return ret;
	}

	return 0;
}

int NANDRestoreFile(const char* filepath, const char* dest) {
	int ret, fd;
	size_t fsize = 0;

	printf("%s ==> %s\n", filepath, dest);
	ret = FAT_GetFileSize(filepath, &fsize);
	
	if (ret < 0) {
		printf("%s: retreiving file size failed (%i)\n", filepath, ret);
		return ret;
	}

	progressbar(0, fsize, 0);

	fd = ret = ISFS_Open(dest, ISFS_OPEN_WRITE);
	if (ret < 0) {
		printf("%s: failed to open (%i)\n", dest, ret);
		return ret;
	}

	FILE* fp = fopen(filepath, "rb");
	if (!fp) {
		printf("%s: ", filepath);
		perror("failed to open");
		return -errno;
	}

	size_t total = 0;


	while (total < fsize) {
		size_t read = fread(fsBuffer, 1, FS_CHUNK, fp);
		if (!read) {
			ret = -errno;
			break;
		}

		ret = ISFS_Write(fd, fsBuffer, read);
		if (ret != read)
			break;
		
		total += ret;
		progressbar(total, fsize, 0);
	}
	ISFS_Close(fd);
	fclose(fp);
	
	putchar('\n');
	if (ret < 0) {
		printf("Error! (%i)\n", ret);
		return ret;
	}

	return 0;
}
