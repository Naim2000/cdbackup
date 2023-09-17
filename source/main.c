#define VERSION "1.2.0"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <gccore.h>
#include <ogc/machine/processor.h>
#include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "fatMounter.h"
#include "tools.h"

#define MAXIMUM(max, size) ( ( size > max ) ? max : size )
#define CHUNK_MAX 1048576 // the Wii menu creates a new cdb.vff by writing 20 * 1M, possibly any other newly created file

#define user_abort 2976579765

static fstats stats ATTRIBUTE_ALIGN(0x20);

const char
	header[] = "cdbackup v" VERSION ", by thepikachugamer\n"
			"Backup/Restore your Wii Message Board data.\n\n",

	cdb_filepath[] = "/title/00000001/00000002/data/cdb.vff",
	sd_filepath[] = "cdbackup.vff";

void* FS_Read(const char* filepath, unsigned int* filesize, int* ec) {
	printf(">> Reading %s from NAND...\n", filepath);

	*ec = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (*ec < 0) return NULL;
	int fd = *ec;

	*ec = ISFS_GetFileStats(fd, &stats);
	if (*ec < 0) {
		ISFS_Close(fd);
		return NULL;
	}
	*filesize = stats.file_length;

	unsigned char* buffer = malloc(*filesize);
	if (!buffer) {
		ISFS_Close(fd);
		*ec = -ENOMEM;
		return NULL;
	}

	unsigned int
		chunks = ceil(*filesize / (double)CHUNK_MAX),
		chunk = 0,
		read_total = 0;
	while(chunk < chunks) {
		unsigned int chunk_size = MAXIMUM(CHUNK_MAX, *filesize - read_total);
		*ec = ISFS_Read(fd, buffer + read_total, chunk_size);
		printf("\r%u / %u bytes... [", read_total + (*ec < 0 ? 0 : *ec), *filesize);
		for(int i = 0; i < (chunks - 1); i++)
			putc(chunk > i ? '=' : ' ', stdout);
		putc(']', stdout);
		fflush(stdout);

		if (*ec < 0) {
			printf(" error! (%d)\n", *ec);
			free(buffer);
			ISFS_Close(fd);
			return NULL;
		} else if (*ec < chunk_size) {
			printf("\nI asked /dev/fs for %u bytes and he came back with %u ...\n", chunk_size, chunk_size - *ec);
			free(buffer);
			ISFS_Close(fd);
			*ec = -EIO;
			return NULL;
		}

		read_total += *ec;
		chunk++;
	}

	printf(" OK!\n");
	ISFS_Close(fd);
	*ec = 0;
	return buffer;
}

void* FAT_Read(const char* filepath, unsigned int* filesize, int* ec) {
	printf(">> Reading %s from FAT device...\n", filepath);

	FILE* fp = fopen(filepath, "rb");
	if (!fp) {
		*ec = -ENOENT;
		return fp;
	};

	fseek(fp, 0, SEEK_END);
	*filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(!*filesize) {
		printf("\x1b[41;30m!>> this file is 0 bytes, is it corrupt?\x1b[40;37m\n");
		*ec = -EINVAL;
		return NULL;
	}

	unsigned char* buffer = malloc(*filesize);
	if(!buffer) {
		*ec = -ENOMEM;
		fclose(fp);
		return buffer;
	};

	unsigned int
		chunks = ceil(*filesize / (double)CHUNK_MAX),
		chunk = 0,
		read_total = 0;
	while(chunk < chunks) {
		unsigned int
			chunk_size = MAXIMUM(CHUNK_MAX, *filesize - read_total),
			read = fread(buffer + read_total, 1, chunk_size, fp);
		printf("\r%u / %u bytes... [", read_total + read, *filesize);
		for(int i = 0; i < (chunks - 1); i++)
			putc(chunk > i ? '=' : ' ', stdout);
		putc(']', stdout);
		fflush(stdout);

		if (read < chunk_size) {
			printf("\nI asked for %u bytes and got %u ...\n", chunk_size, read);
			*ec = -EIO;
			free(buffer);
			fclose(fp);
			return NULL;
		}

		read_total += read;
		chunk++;
	}
	fclose(fp);
	printf(" OK!\n");
	return buffer;

}

int FS_Write(const char* filepath, unsigned char* buffer, unsigned int filesize) {
	printf(">> Writing to %s on NAND...\n", filepath);

	int ret = 0;
	int fd = ISFS_Open(filepath, ISFS_OPEN_WRITE);
	if (fd < 0) return fd;

	/*
	int ret = ISFS_Write(fd, buffer, filesize);
	if (ret < filesize)
		ret = ret < 0 ? ret : -EIO
	*/

	unsigned int
		chunks = ceil(filesize / (double)CHUNK_MAX),
		chunk = 0,
		written = 0;
	while (chunk < chunks) {
		unsigned int chunk_size = MAXIMUM(CHUNK_MAX, filesize - written);
		ret = ISFS_Write(fd, buffer + written, chunk_size);
		printf("\r%u / %u bytes... [", written + (ret < 0 ? 0 : ret) , filesize); // ret & (1 << 31) <-- why did i write that
		for(int i = 0; i < (chunks - 1); i++)
			putc(chunk > i ? '=' : ' ', stdout);
		putc(']', stdout);
		fflush(stdout);
		if (ret < 0) {
			printf(" error! (%d)\n", ret);
			ISFS_Close(fd);
			return ret;
		} else if (ret < chunk_size) {
			printf("\nI gave /dev/fs %u bytes and he came back with %u left ...\n\n", chunk_size, chunk_size - ret);
			ISFS_Close(fd);
			return -EIO;
		}
		written += ret;
		chunk++;
	}

	printf(" OK!\n\n");
	ISFS_Close(fd);
	return ret;
}

int FAT_Write(const char* filepath, unsigned char* buffer, unsigned int filesize) {
	printf(">> Writing to %s on FAT device...\n", filepath);

	FILE *fp = fopen(filepath, "wb");
	if (!fp) return ~0;

	unsigned int
		chunks = ceil(filesize / (double)CHUNK_MAX),
		chunk = 0,
		written = 0;
	while (chunk < chunks) {
		unsigned int chunk_size = MAXIMUM(CHUNK_MAX, filesize - written);
		size_t wrote = fwrite(buffer + written, 1, chunk_size, fp);
		printf("\r%u / %u bytes... [", written + wrote, filesize);
		for(int i = 0; i < (chunks - 1); i++)
			putc(chunk > i ? '=' : ' ', stdout);
		putc(']', stdout);
		fflush(stdout);
		if (wrote < chunk_size) {
			printf("\nfwrite() came back with %u bytes left...\n\n", wrote);
			fclose(fp);
			return -EIO;
		}
		written += wrote;
		chunk++;
	}

	printf(" OK!\n\n");
	fclose(fp);
	return 0;
}

int backup() {
	int ret = 0;
	unsigned int filesize;

	FILE* fp = fopen(sd_filepath, "rb");
	if (fp) {
		fclose(fp);
		fp = NULL;
		if(!confirmation("Backup file appears to exist; overwrite it?\n", 3)) return user_abort;
	}
	putc('\n', stdout);

	unsigned char* buffer = FS_Read(cdb_filepath, &filesize, &ret);
	if(ret < 0) {
		printf("Error reading message board data! (%d)\n", ret);
		return ret;
	}

	ret = FAT_Write(sd_filepath, buffer, filesize);
	if (ret < 0) {
		printf("Error writing backup file! (%d)\n", ret);
		return ret;
	}

	free(buffer);
	printf("All done!\n");
	return 0;
}

int restore() {
	int ret = 0;
	unsigned int filesize;

	if(!confirmation("Are you sure you want to restore your message board data backup?\n", 3)) return user_abort;

	unsigned char* buffer = FAT_Read(sd_filepath, &filesize, &ret);
	if(ret < 0) {
		printf("Error reading backup file! (%d)\n", ret);
		if(ret == -ENOENT)
			printf("(does it exist?)\n");
		return ret;
	}

	if(memcmp(buffer, "VFF ", 4)) {
		printf("\x1b[41;30m this isn't a VFF file... [0x%08x != 0x%08x (\"VFF \")] \x1b[40;37m", *(unsigned int*)buffer, 0x56464620);
		free(buffer);
		return -EINVAL;
	}

	ret = FS_Write(cdb_filepath, buffer, filesize);
	if (ret < 0) {
		printf("Error writing backup file! (%d)\n", ret);
		if(ret == -106) {
			printf("* Please don't delete the data before restoring your backup...\n");
			sleep(1);

			printf("Press any button to return to the Wii Menu.");
			while(!wii_down) { input_scan(); VIDEO_WaitVSync(); }
			SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
		}
		return ret;
	}

	free(buffer);
	printf("All done!\n");
	return 0;
}

int delete() {
	if(!confirmation("Are you sure you want to delete your message board data??\n", 6)) return user_abort;

	int ret = ISFS_Delete(cdb_filepath);
	if (ret == -106) { // ENOENT
		printf("\nYou already deleted it. (%d)", ret);
		sleep(3);
		ret = 0;
	}
	else if (ret < 0)
		printf("Error deleting %s! (%d)", cdb_filepath, ret);
	else
		printf("Deleted %s.\n", cdb_filepath);

	return ret;
}

int main() {
	int ret = ~1;

	init_video(2, 0);
	printf(header);
	WPAD_Init();
	PAD_Init();
	ISFS_Initialize(); // i'll just hit enotinit on the attempt to open. dont care

	if (MountSD() > 0)
		chdir("sd:/");
	else if (MountUSB())
		chdir("usb:/");
	else {
		printf("Could not mount any storage device!\n");
		return quit(~0);
	}

	sleep(2);
	printf(
		"Press A to backup your message board data.\n"
		"Press +/Y to restore your message board data.\n"
		"Press -/X to delete your message board data.\n"
		"Press HOME/START to return to loader.\n\n");

	while(ret == ~1) {
		input_scan();
		if (input_pressed(input_a))
			ret = backup();

		else if (input_pressed(input_start))
			ret = restore();

		else if (input_pressed(input_select))
			ret = delete();

		else if (input_pressed(input_home))
			return user_abort;

		VIDEO_WaitVSync();
	}
	return quit(ret);
}
