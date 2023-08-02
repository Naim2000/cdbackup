#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
// #include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <malloc.h>

#include "fatMounter.h"

#define no_memory		-4011
#define short_read		-4022
#define short_write		-4021
#define fat_open_failed	-4111
#define ok				0

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

typedef struct {
	size_t short_actual;
	u32 short_expected;
} errinfo;

void init_video(int row, int col) {
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[%d;%dH", row, col);
}

s32 read_nand_file(const char* filepath, u8* *buffer, u32* filesize, errinfo* error_info) {
	s32 ret;
	fstats stats ATTRIBUTE_ALIGN(32);

	s32 fd = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (fd < 0) return fd;

	ret = ISFS_GetFileStats(fd, &stats);
	if (ret < 0) {
		ISFS_Close(fd);
		return ret;
	}
	*filesize = stats.file_length;

	*buffer = (u8*)malloc(*filesize);
	if (*buffer == NULL) {
		ISFS_Close(fd);
		return no_memory;
	}

	ret = ISFS_Read(fd, *buffer, *filesize);
	if (ret <= 0) {
		ISFS_Close(fd);
		return(ret);
	}
	else if (ret < *filesize) {
		error_info->short_expected = *filesize;
		error_info->short_actual   = ret;
		ISFS_Close(fd);
		return short_read;
	}
	return 0;
}

s32 write_fat_file(const char* filepath, u8 *buffer, u32 filesize, errinfo* error_info) {
	FILE *fd = fopen(filepath, "wb");
	if (fd == NULL) return fat_open_failed;

	size_t written = fwrite(buffer, 1, filesize, fd);
	if(written < filesize) {
		error_info->short_expected = filesize;
		error_info->short_actual   = written;
		return short_write;
	}
	fflush(fd);
	fclose(fd);
	return ok;
}

int main() {
	static char nand_file_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/cdb.vff";
	static char sd_file_path[] = "sd:/cdbackup.vff";
	s32 ret;
	u8 *nand_file_buffer	= NULL;
	size_t nand_file_size;
	errinfo error_info;

	init_video(2, 0);

	ret = ISFS_Initialize();
	printf("ISFS_Initialize -> %d\n", ret);
	sleep(2);
	if (ret < 0) return ret;

	printf("reading %s ...\n", nand_file_path);

	ret = read_nand_file(nand_file_path, &nand_file_buffer, &nand_file_size, &error_info);
	switch(ret) {
		case no_memory:
			printf("Could not allocate %d bytes of memory!\n", nand_file_size);
			sleep(2);
			return ret;
		case short_write:
			printf("Short read! Only got %d bytes out of %d.\n", error_info.short_actual, error_info.short_expected);
			sleep(2);
			return ret;
		case ok:
			printf("OK! Read %d bytes.\n", nand_file_size);
			break;

		default:
			printf("Error while reading file! (%d)\n", ret);
			sleep(2);
			return ret;
	}

	// printf("%s is %d bytes, read %d bytes.", nand_file_path, *nand_file_size, ret);
	sleep(2);

	printf("writing to %s...\n", sd_file_path);
	sleep(2);

	if(MountSD() < 0) {
		printf("failed to mount SD.\n");
		sleep(2);
		return(-6);
	}

	ret = write_fat_file(sd_file_path, nand_file_buffer, nand_file_size, &error_info);
	switch(ret) {
		case fat_open_failed:
			printf("Could not open handle to file!\n");
			sleep(2);
			return ret;
		case short_write:
			printf("Short write! Only got in %d bytes out of %d; do you have enough free space?\n", error_info.short_actual, error_info.short_expected);
			sleep(2);
			return ret;
		case ok:
			printf("OK! Wrote %d bytes.\n", nand_file_size);
			break;
	}

	printf("freeing buffer...\n");
	free(nand_file_buffer);
	printf("shutting down ISFS...\n");
	ISFS_Deinitialize();
	printf("unmounting SD...");
	UnmountSD();

	printf("all done!\n");
	sleep(5);

	return 0;
}
