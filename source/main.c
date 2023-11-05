#define VERSION "1.2.1"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "fatMounter.h"
#include "tools.h"
#include "fs.h"

#define user_abort 2976579765

const char* header = "cdbackup v" VERSION ", by thepikachugamer\nBackup/Restore your Wii Message Board data.\n\n",
	*cdb_filepath = "/title/00000001/00000002/data/cdb.vff",
	*sd_filepath = "cdbackup.vff";

static char* pwd() {
	static char path[0x400];
	return getcwd(path, sizeof(path));
}

int backup() {
	int ret;
	size_t filesize = 0;
	unsigned char* buffer;
	FILE* fp;
	if ((fp = fopen(sd_filepath, "rb"))) {
		fclose(fp);
		if(!confirmation("Backup file appears to exist; overwrite it?\n", 3)) return user_abort;
	}
	putchar('\n');

	printf(">> Reading %s from NAND...\n", cdb_filepath);
	ret = FS_Read(cdb_filepath, &buffer, &filesize, &progressbar);
	if (ret < 0) {
		printf("Error! (%d)\n", ret);
		return ret;
	}
	printf("OK!\n");


	printf(">> Writing %s to %s...\n", sd_filepath, pwd());
	ret = FAT_Write(sd_filepath, buffer, filesize, &progressbar);
	if (ret < 0) {
		printf("Error! (%d)\n", ret);
		return ret;
	}
	printf("OK!\n");

	free(buffer);
	printf("All done!\n");
	return 0;
}

int restore() {
	int ret;
	size_t filesize = 0;
	unsigned char* buffer;

	if(!confirmation("Are you sure you want to restore your message board data backup?\n", 3)) return user_abort;

	printf("Reading %s from %s...\n", sd_filepath, pwd());
	ret = FAT_Read(sd_filepath, &buffer, &filesize, &progressbar);
	if(ret < 0) {
		printf("Error! (%d)\n", ret);
		if (ret == -ENOENT)
			printf("\"No such file or directory\".\n");

		return ret;
	}
	printf("OK!\n");

	if(memcmp(buffer, "VFF ", 4) != 0) {
		printf("\x1b[41;30m this isn't a VFF file... [0x%08x != 0x%08x (\"VFF \")] \x1b[40;37m", *(unsigned int*)buffer, 0x56464620);
		free(buffer);
		return -EINVAL;
	}

	printf("Writing to %s on NAND...\n", cdb_filepath);
	ret = FS_Write(cdb_filepath, buffer, filesize, &progressbar);
	if (ret < 0) {
		printf("Error! (%d)\n", ret);
		if(ret == -106) {
			printf("\nPlease don't delete the data before restoring your backup...\n");
			sleep(1);

			printf("Press any button to return to the Wii Menu.");
			input_scan();
			while(!wii_down) { input_scan(); VIDEO_WaitVSync(); }
			SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
		}
		return ret;
	}
	printf("OK!\n");

	free(buffer);
	printf("All done!\n");
	return 0;
}

int delete() {
	if(!confirmation("Are you sure you want to delete your message board data??\n", 6)) return user_abort;

	printf (">> Deleting %s from NAND...\n", cdb_filepath);
	int ret = ISFS_Delete(cdb_filepath);
	if (ret == -106) {
		printf("You already deleted it. (%d)\n", ret);
		ret = 0;
	}
	else if (ret < 0)
		printf("Error! (%d)", ret);
	else
		printf("OK!\n");

	return ret;
}

int main() {
	int ret = ~1;

	printf(header);
	WPAD_Init();
	PAD_Init();
	ISFS_Initialize();

	if (!mountSD() && !mountUSB()) {
		printf("Could not mount any storage device!\n");
		quit(-ENXIO);
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
	quit(ret);

	return 0;
}
