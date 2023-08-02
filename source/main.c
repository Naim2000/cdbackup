#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
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

static char header[] = "cdbackup : Backup/restore Wii Message Board data\n\n";
static char cdb_filepath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/cdb.vff";
static char sd_filepath[] = "cdbackup.vff";

typedef struct {
	size_t short_actual;
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
		error_info->short_actual   = ret;
		ISFS_Close(fd);
		return short_read;
	}
	return 0;
}

s32 read_fat_file(const char filepath[], u8* *buffer, u32 *filesize, errinfo *error_info) {
	FILE *fd = fopen(filepath, "rb");
	if (fd == NULL) return fat_open_failed;

	fseek(fd, 0, SEEK_END);
	*filesize = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	*buffer = (u8*)malloc(*filesize);
	if(*buffer == NULL) return no_memory;

	size_t read = fread(*buffer, 1, *filesize, fd); // size_t --> unsigned long, not gonna return negative
	if(read < *filesize) {
		error_info->short_actual = read;
		return short_read;
	}
	return ok;
}

s32 write_nand_file(const char filepath[ISFS_MAXPATH], u8 *buffer, u32 filesize, errinfo *error_info) {
	s32 fd = ISFS_Open(filepath, ISFS_OPEN_WRITE);
	if (fd < 0) return fd;

	s32 ret = ISFS_Write(fd, buffer, filesize);
	if (ret < 0) {
		ISFS_Close(fd);
		return ret;
	}
	else if (ret < filesize) {
		error_info->short_actual = ret;
		ISFS_Close(fd);
		return short_write;
	}
	return ok;
}

s32 write_fat_file(const char* filepath, u8 *buffer, u32 filesize, errinfo* error_info) {
	FILE *fd = fopen(filepath, "wb");
	if (fd == NULL) return fat_open_failed;

	size_t written = fwrite(buffer, 1, filesize, fd);
	if(written < filesize) {
		error_info->short_actual = written;
		fclose(fd);
		return short_write;
	}
	fflush(fd);
	fclose(fd);
	return ok;
}

s32 backup() {
	s32 ret;
	u8 *fb = NULL;
	size_t filesize;
	errinfo error_info;

	printf("reading %s ...\n", cdb_filepath);

	ret = read_nand_file(cdb_filepath, &fb, &filesize, &error_info);
	switch(ret) {
		case no_memory:
			printf("Could not allocate %d bytes of memory!\n", filesize);
			sleep(5);
			return ret;
		case short_read:
			printf("Short read! Only got %d bytes out of %d.\n", error_info.short_actual, filesize);
			sleep(5);
			return ret;
		case ok:
			printf("OK! Read %d bytes.\n", filesize);
			break;

		default:
			printf("Error while reading file! (%d)\n", ret);
			sleep(5);
			return ret;
	}

	printf("writing to %s ...\n", sd_filepath);

	ret = write_fat_file(sd_filepath, fb, filesize, &error_info);
	switch(ret) {
		case fat_open_failed:
			printf("Could not open handle to file!\n");
			sleep(5);
			return ret;
		case short_write:
			printf("Short write! Only got in %d bytes out of %d; do you have enough free space?\n", error_info.short_actual, filesize);
			sleep(5);
			return ret;
		case ok:
			printf("OK! Wrote %d bytes.\n", filesize);
			sleep(5);
			break;
	}

	free(fb);
	return ok;
}

s32 restore() {
	s32 ret;
	u8 *fb = NULL;
	size_t filesize;
	errinfo error_info;

	printf("reading %s ...\n", sd_filepath);
	ret = read_fat_file(sd_filepath, &fb, &filesize, &error_info);
	switch(ret) {
		case fat_open_failed:
			printf("Could not open handle to file; does it exist?\n");
			sleep(5);
			return ret;
		case no_memory:
			printf("Could not allocate %d bytes of memory!", filesize);
			sleep(5);
			return ret;
		case short_read:
			printf("Short read! Only got %d bytes out of %d.\n", error_info.short_actual, filesize);
			sleep(5);
			return ret;
		case ok:
			printf("OK! Read %d bytes.\n", filesize);
			sleep(5);
			break;
	}

	printf("writing to %s ...\n", cdb_filepath);
	ret = write_nand_file(cdb_filepath, fb, filesize, &error_info);
	switch(ret) {
		case short_write:
			printf("Short write! Only got %d bytes out of %d.\n", error_info.short_actual, filesize);
			sleep(5);
			return ret;
		case ok:
			printf("OK! Wrote %d bytes.\n", filesize);
			sleep(5);
			break;

		default:
			printf("Error while writing file! (%d)\n", ret);
			sleep(5);
			return ret;
	}

	free(fb);
	return ok;
}

int main() {
	init_video(2, 0);
	WPAD_Init();
	PAD_Init();
	s32 ret = ISFS_Initialize();
	if (ret < 0) {
		printf("ISFS_Initialize returned %d.\nNever seen this happen, however, it just did. Always prepare for the worst.\n", ret);
		return ret;
	}

	if (MountSD() > 0) chdir("sd:/");
	else if (MountUSB()) chdir("usb:/");
	else {
		printf("Could not mount storage device!\n");
		return -1;
	}

	printf(header);
	sleep(3);
	printf("Press A to backup your message board data.\n");
	printf("Press -/Y to restore your message board data.\n");
	printf("Press HOME/START to return to loader.\n\n");
	while(true) {
		WPAD_ScanPads();
		PAD_ScanPads();
		u32 wii_down = WPAD_ButtonsDown(0);
		u32 gcn_down =  PAD_ButtonsDown(0);
		if		(wii_down & WPAD_BUTTON_A		|| gcn_down & PAD_BUTTON_A)		return backup();
		else if	(wii_down & WPAD_BUTTON_MINUS	|| gcn_down & PAD_BUTTON_Y)		return restore();
		else if	(wii_down & WPAD_BUTTON_HOME	|| gcn_down & PAD_BUTTON_START)	return ok;
		VIDEO_WaitVSync();
	}
}
