#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <fat.h>
#include <ogc/isfs.h>

// #include "id.h"
#include "wiibasics.h"
#include "runtimeiospatch.h"

#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))
#define SYSMENU_ID			TITLE_ID(1, 2)

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	u64 tid;
	static fstats stats ATTRIBUTE_ALIGN(32);
	static char filepath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	s32 ret, fd;
	char *buffer = NULL;
	u32 *filesize = 0;
	FILE *cdbackup = NULL;

	basicInit();
	if (!AHBPROT_DISABLED) {
		printf("Hardware protection is enabled. Leaving ...");
		return ERROR_AHBPROT;
	}
	printf("Hardware protection is disabled. Patching IOS...\n");
	ret = IosPatch_RUNTIME(true, false, false, false);
	if(ret < 0) {
		printf("Unknown error!");
		return ret;
	}
	printf("OK!\n");
	ISFS_Initialize();
	ES_GetTitleID(&tid);
	printf("I am %08x-%08x\n", TITLE_UPPER(tid), TITLE_LOWER(tid));
	if (tid != SYSMENU_ID) {
		printf("^^ Not system menu!!"); // maybe i should try identify as sm here
		sleep(2);
		return(-1);
	}
	sleep(2);

	ES_GetDataDir(tid, &filepath);
	printf("data directory: %s\n", filepath);
	sleep(1);
	strcat(filepath, "/cdb.vff");
	printf("file path: %s\n", filepath);
	sleep(1);

	fd = ISFS_Open(filepath, ISFS_OPEN_READ);
	printf("ISFS_Open -> %d", fd);
	if (fd <= 0) {
		sleep(2);
		return fd;
	}
	ret = ISFS_GetFileStats(fd, &stats);
	printf("ISFS_GetFileStats -> %d", ret);
	if (ret < 0) {
		sleep(2);
		return ret;
	}
	*filesize = stats.file_length;
	buffer = (char*)malloc(*filesize);
	if (buffer == NULL) {
		printf("Failed to allocate %d bytes of memory!", *filesize);
		sleep(2);
		return -1;
	}
	ret = ISFS_Read(fd, buffer, *filesize);
	if (ret < 0) {
		printf("Couldn't read cdb.vff . %d", ret);
		sleep(2);
		return ret;
	}

	if(!fatInitDefault()) {
		printf("Couldn't initialize FAT device.");
		sleep(2);
		return -1;
	}
	cdbackup = fopen("sd:/cdb.vff", "w");
	if (!cdbackup) {
		printf("Couldn't open backup file.");
		sleep(2);
		return -1;
	}
	fprintf(cdbackup, buffer);
	fflush(cdbackup);
	fclose(cdbackup);
	printf("Done, probably.");
	return 0;
}
