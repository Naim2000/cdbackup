#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
// #include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <malloc.h>

#include "fatMounter.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

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

int main() {
	static char nand_file_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/cdb.vff";
	static char sd_file_path[] = "sd:/cdbackup.vff";
	static fstats nand_file_stats ATTRIBUTE_ALIGN(32);
	s32 ret, nand_file_handle;
	u32 *nand_file_size		= NULL;
	u8 *nand_file_buffer	= NULL;
	FILE *sd_file_handle	= NULL;
	size_t sd_file_write;

	init_video(2, 0);

	ret = ISFS_Initialize();
	printf("ISFS_Initialize -> %d\n", ret);
	sleep(2);
	if (ret < 0) return ret;

	printf("file path: %s\n", nand_file_path);

	nand_file_handle = ISFS_Open(nand_file_path, ISFS_OPEN_READ);
	printf("ISFS_Open -> %d\n", nand_file_handle);
	sleep(2);
	if (nand_file_handle <= 0) return(-1);

	ret = ISFS_GetFileStats(nand_file_handle, &nand_file_stats);
	printf("ISFS_GetFileStats -> %d\n", ret);
	sleep(2);
	if (ret < 0) return(-2);
	nand_file_size = &nand_file_stats.file_length;
	printf("%s is %d bytes long.\n", nand_file_path, *nand_file_size);
	sleep(2);

	nand_file_buffer = (u8*)malloc(*nand_file_size);
	if (nand_file_buffer == NULL) {
		printf("memory allocation failed! %d\n", *nand_file_size);
		sleep(2);
		return(-3);
	}
	printf("allocated %d bytes of memory\n", *nand_file_size);
	sleep(2);

	ret = ISFS_Read(nand_file_handle, nand_file_buffer, *nand_file_size);
	printf("ISFS_Read -> %d\n", ret);
	sleep(2);
	if (ret <= 0) return(-4);
	else if (ret < *nand_file_size) {
		printf("short read (%d/%d)\n", ret, *nand_file_size);
		sleep(2);
		return(-5);
	}

	// printf("%s is %d bytes, read %d bytes.", nand_file_path, *nand_file_size, ret);
	sleep(2);

	printf("next step: writing the backup\n");
	sleep(2);

	if(MountSD() < 0) {
		printf("failed to mount SD.\n");
		sleep(2);
		return(-6);
	}

	sd_file_handle = fopen(sd_file_path, "wb");
	printf("fopen --> %p\n", sd_file_handle);
	sleep(2);
	if (sd_file_handle == NULL) return(-7);

	sd_file_write = fwrite(nand_file_buffer, 1, *nand_file_size, sd_file_handle);
	printf("fwrite -> %d\n", sd_file_write);
	if(sd_file_write < *nand_file_size) {
		printf("short write (%d/%d)\n", sd_file_write, *nand_file_size);
		return(-8);
	}

	printf("flushing changes...\n");
	fflush(sd_file_handle);
	printf("closing backup file...\n");
	fclose(sd_file_handle);
	printf("closing nand file...\n");
	ISFS_Close(nand_file_handle);
	printf("closing ISFS...\n");
	ISFS_Deinitialize();

	printf("all done!\n");
	sleep(5);

	return 0;
}
