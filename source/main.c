#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <ogc/machine/processor.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <libpatcher.h>
#include <fat.h>

#include "config.h"

#include "fatMounter.h"
#include "tools.h"
#include "menu.h"
#include "fs.h"
#include "vff.h"
#include "cdbfile.h"

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

static int export_cb(const char* path, FILINFO* st)
{
	int res;

	uint32_t sendtime_x = 0;
	time_t sendtime;
	struct tm tm_sendtime = {};

	FIL fp[1];
	struct CDBAttrHeader* cdbattr = NULL;
	struct CDBFILE* cdbfile = NULL;
	char outpath[PATH_MAX];
	FILE* text = NULL;

	if (!strcmp(st->fname, "cdb.conf")) return 0;

	if (!sscanf(st->fname, "%X.000", &sendtime_x))
	{
		printf("%s: Invalid file name(?)\n", path);
		return -EINVAL;
	}

	sendtime = SECONDS_TO_2000 + sendtime_x;
	gmtime_r(&sendtime, &tm_sendtime);

	cdbattr = malloc(st->fsize);
	if (!cdbattr)
	{
		printf("%s: malloc failed (%u)\n", path, st->fsize);
		return -ENOMEM;
	}
	cdbfile = cdbattr->cdbfile;

	res = f_open(fp, path, FA_READ);
	if (res != FR_OK)
	{
		printf("%s: f_open() failed (%i)\n", path, res);
		goto finish;
	}

	UINT read;
	res = f_read(fp, cdbattr, st->fsize, &read);
	f_close(fp);
	if (read != st->fsize) // imagine xor
	{
		printf("%s: short read (%i : %u/%u)\n", path, res, read, st->fsize);
		goto finish;
	}

	char timestr[30];
	char datetimestr[60];
	static const char mon[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	strftime(datetimestr, sizeof(datetimestr), "%F %r", &tm_sendtime);
	strftime(timestr, sizeof(timestr), "%p %I.%M.%S", &tm_sendtime);
	sprintf(outpath, "%s:/cdb/%i/%s/%02i/%s %s.txt", GetActiveDeviceName(), tm_sendtime.tm_year + 1900, mon[tm_sendtime.tm_mon],
			tm_sendtime.tm_mday, timestr, cdbattr->description);

	printf("%s %s from %s\n", datetimestr, cdbattr->description, cdbfile->sender);

	char* ptr = strchr(outpath, '/') + 1;

	while ((ptr = strchr(ptr, '/')))
	{
		ptr[0] = 0;
		if (mkdir(outpath, 0644) < 0 && errno != EEXIST) goto error;
		ptr[0] = '/';
		ptr++;
	}

	text = fopen(outpath, "wb");
	if (!text) goto error;;
	static uint16_t bom[] = { 0xFEFF }, newlines[] = { '\n', '\n' };
	fwrite(bom, 1, sizeof(bom), text);

	uint16_t* start;
	unsigned count;
	for (start = (void*)cdbfile + cdbfile->desc_offset, count = 0;
		 start[count] != 0x0000;
		 count++);

	if (!fwrite(start, sizeof(uint16_t), count, text)) goto error;

	if (!fwrite(newlines, sizeof(newlines), 1, text)) goto error;

	for (start = (void*)cdbfile + cdbfile->body_offset, count = 0;
		 start[count] != 0x0000;
		 count++);

	if (!fwrite(start, sizeof(uint16_t), count, text)) goto error;

	fclose(text);

	if (cdbfile->attachment_offset)
	{
		const char* ext;
		uint32_t attachment_magic = (*(uint32_t*)((void*)cdbfile + cdbfile->attachment_offset));
		switch (attachment_magic)
		{
			case 0x55AA382D: ext = "U8";	break;
			case 0x30335F30: ext = "ptm";	break;
			case 0x414A5047: ext = "ajpg";	break;

			default: ext = "bin"; break;
		}

		sprintf(strrchr(outpath, '.'), "-attachment.%s", ext);
		text = fopen(outpath, "wb");
		if (!text) goto error;

		if (!fwrite((void*)cdbfile + cdbfile->attachment_offset, cdbfile->attachment_size, 1, text)) goto error;
		fclose(text);
	}

	goto finish;


error:
	res = -errno;
	perror(outpath);

finish:
	free(cdbattr);
	if (text) fclose(text);
	return res;
}

static int copytree(const char* src, int (*callback)(const char*, FILINFO*))
{
	char path[256];

	FRESULT fres;
	DIR dp[1] = {};
	FILINFO st;

	fres = f_opendir(dp, src);
	if (fres != FR_OK) {
		printf("f_opendir failed! (%i)\n", fres);
		return fres;
	}

	while ( ((fres = f_readdir(dp, &st)) == FR_OK) && st.fname[0] )
	{
		sprintf(path, "%s/%s", src, st.fname);

		if (st.fattrib & AM_DIR)
			copytree(path, callback);

		else
			callback(path, &st);
	}

	f_closedir(dp);

	return fres;
}

int export(void)
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

	ret = copytree("vff:", export_cb);
	puts("\nOK!");

	f_unmount("vff:");
	fclose(fp);
	return ret;
}


static MainMenuItem items[5] =
{
	{
		"Backup",
		"\x1b[32;1m", // light green
		backup
	},

	{
		"Restore",
		"\x1b[31;1m", // light red
		restore
	},

	{
		"Delete",
		"\x1b[41;1m\x1b[30m", // Black on light red
		delete
	},

	{
		"Export (text)",
		"\x1b[33;1m", // light yellow
		export
	},

	{
		"Exit",
		"",
		NULL
	}
};

int main() {
	WPAD_Init();
	PAD_Init();
	ISFS_Initialize();

	if (!patch_isfs_permissions())
	{
		printf("Failed to patch NAND permissions, deleting is not going to work...\n\n");
		sleep(2);
	}


	if (!FATMount())
		quit(-ENODEV);

	quit(MainMenu(items, 5));


	return 0;
}
