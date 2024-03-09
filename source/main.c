#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <runtimeiospatch.h>
#include <fat.h>

#include "config.h"

#include "fatMounter.h"
#include "tools.h"
#include "fs.h"
#include "vff.h"

int backup() {
	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	if (!FAT_GetFileSize(filepath, 0)) {
		if (!confirmation("Backup file already exists, do you want to overwrite it?", 2))
			return user_abort;
	}

	return NANDBackupFile(NAND_TARGET, filepath);
}

int restore() {
	size_t filesize = 0;
	struct VFFHeader header = {};

	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	if (FAT_GetFileSize(filepath, &filesize) < 0) {
		puts("Failed to get backup file size, does it exist?");
		perror("Error details");

		return -errno;
	}

	if (FAT_Read(filepath, &header, sizeof(header), 0, 0) < 0) {
		puts("Failed to read VFF header!");
		perror("Error details");

		return -errno;
	}

	if (!sanityCheckVFFHeader(&header, filesize)) {
		puts("Bad VFF header!");

		return -EBADF;
	}

	if (!confirmation("Are you sure you want to restore your backup?", 6))
		return user_abort;

	return NANDRestoreFile(filepath, NAND_TARGET);
}

int delete() {
	int ret;
	if ((ret = NAND_GetFileSize(NAND_TARGET, 0)) < 0) {
		printf("Failed to get file size**, did you delete it already? (%i)\n", ret);
		return ret;
	}

	if(!confirmation("Are you sure you want to delete your message board data??", 6))
		return user_abort;

	printf (">> Deleting %s from NAND... ", NAND_TARGET);
	ret = ISFS_Delete(NAND_TARGET);
	if (ret == -106) {
		puts("You already deleted it.");
		return 0;
	}

	else if (ret < 0) printf("Error! (%d)", ret);
	else printf("OK!\n");

	return ret;
}

static int copytree(const char* src, const char* dst)
{
	char path[256], pathB[256];
	unsigned char buffer[0x200];

	FRESULT fres;
	DIR dp[1] = {};
	FILINFO st;

	puts(src);

	fres = f_opendir(dp, src);
	if (fres != FR_OK) {
		printf("f_opendir failed! (%i)\n", fres);
		return fres;
	}

	mkdir(dst, 0644);

	while ( ((fres = f_readdir(dp, &st)) == FR_OK) && st.fname[0] )
	{
		sprintf(path,  "%s/%s", src, st.fname);
		sprintf(pathB, "%s/%s", dst, st.fname);

		if (st.fattrib & AM_DIR)
		{
			copytree(path, pathB);
		}
		else
		{
			FIL fp[1];

			puts(path);

			fres = f_open(fp, path, FA_READ);
			if (fres != FR_OK)
			{
				printf("%s: f_open() failed (%i)\n", path, fres);
				break;
			}

			FILE* fp_B = fopen(pathB, "wb");
			if (!fp_B)
			{
				perror(pathB);
				f_close(fp);
				break;
			}

			UINT read, left = st.fsize;
			while ( ((fres = f_read(fp, buffer, sizeof(buffer), &read)) == FR_OK) && read)
			{
				if (!fwrite(buffer, read, 1, fp_B))
				{
					perror(pathB);
					break;
				}
				left -= read;
			}

			f_close(fp);
			fclose(fp_B);

			if (left)
				break;
		}
	}

	f_closedir(dp);

	return fres;
}

int extract(void)
{
	FATFS fs = {};

	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	FILE* fp = fopen(filepath, "rb");
	if (!fp) {
		perror(filepath);
		return -errno;
	}

	f_mount(&fs, "vff:", 0);

	int ret = VFFInit(fp, &fs);
	if (ret < 0)
		return ret;

	sprintf(filepath, "%s:/cdb", GetActiveDeviceName());
	ret = copytree("vff:", filepath);
	printf("OK!\n");

	f_unmount("vff:");
	return ret;
}

int main() {

	puts(
		"cdbackup " VERSION ", by thepikachugamer\n"
		"Backup/Restore your Wii Message Board data.\n");

	WPAD_Init();
	PAD_Init();
	ISFS_Initialize();

	if (!FATMount())
		quit(-ENODEV);

	if (IosPatch_RUNTIME(nand_permissions, false) < 0)
		printf("Failed to patch NAND permissions, deleting is not going to work...\n\n");

	printf(
		"Press A to backup your message board data.\n"
		"Press +/Y to restore your message board data.\n"
		"Press -/X to delete your message board data.\n"
		"Press 1 to extract VFF.\n"
		"Press HOME/START to return to loader.\n\n");

	while (true) {
		input_scan();
		if (input_pressed(input_a)) quit(backup());
		else if (input_pressed(input_start)) quit(restore());
		else if (input_pressed(input_select)) quit(delete());
		else if (input_pressed(input_x)) quit(extract());

		else if (input_pressed(input_home))
			return user_abort;

		VIDEO_WaitVSync();
	}

	return 0;
}
