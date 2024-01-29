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

/* This will be put somewhere better soon enough */
struct VFFHeader {
	uint32_t magic;			// 'VFF ' 0x56464620
	uint16_t BOM;			// 0xFEFF ("never checked"?)
	uint16_t length;		// Length of...... of what
	uint32_t fileSize;
	uint16_t size;			// 0x0020

	char padding[18];
};
static_assert(sizeof(struct VFFHeader) == 0x20, "Wrong size for VFF header!");

static inline bool sanityCheckVFFHeader(struct VFFHeader hdr, size_t filesize) {
	return
		hdr.magic		== 0x56464620
	&&	hdr.BOM			== 0xFEFF
//	&&	length			== 0x0100
	&&	hdr.fileSize	== filesize
	&&	hdr.size		== 0x20;
}

int backup() {
	int ret = 0;
	size_t filesize = 0;

	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	if (!FAT_GetFileSize(filepath, 0)) {
		if (!confirmation("Backup file already exists, do you want to overwrite it?", 2))
			return user_abort;
	}

	return NANDBackupFile(NAND_TARGET, filepath);
}

int restore() {
	int ret = 0;
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

	if (!sanityCheckVFFHeader(header, filesize)) {
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

int main() {

	puts("cdbackup " VERSION ", by thepikachugamer");
	puts("Backup/Restore your Wii Message Board data.");
	puts("");

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
		"Press HOME/START to return to loader.\n\n");

	while (true) {
		input_scan();
		if (input_pressed(input_a)) quit(backup());
		else if (input_pressed(input_start)) quit(restore());
		else if (input_pressed(input_select)) quit(delete());

		else if (input_pressed(input_home))
			return user_abort;

		VIDEO_WaitVSync();
	}


	return 0;
}
