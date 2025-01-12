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
#include "libpatcher.h"
#include <fat.h>
#include "odh.h"
#include "dirent.h"
#include "aes.h"
#include <setjmp.h>

#include "config.h"

#include "fatMounter.h"
#include "menu.h"
#include "fs.h"
#include "vff.h"
#include "cdbfile.h"

#include "converter/converter.h"

void export_sd(char *name);

__attribute__((weak)) void OSReport(const char *, ...) {};

bool confirmation(const char *message, unsigned int wait_time)
{
	puts(message);
	while (wait_time)
	{
		printf("\x1b[30;1m%u...\x1b[39m\r", wait_time--);
		sleep(1);
	}

	puts("Press +/START to confirm.\n"
		 "Press any other button to cancel.\n");

	return (bool)(wait_button(0) & WPAD_BUTTON_PLUS);
}

int backup()
{
	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	if (!FAT_GetFileSize(filepath, 0))
	{
		if (!confirmation("Backup file already exists, do you want to overwrite it?", 2))
			return user_abort;
	}

	return NANDBackupFile(NAND_TARGET, filepath);
}

int restore()
{
	size_t filesize = 0;
	struct VFFHeader header = {};

	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	if (FAT_GetFileSize(filepath, &filesize) < 0)
	{
		puts("Failed to get backup file size, does it exist?");
		perror("Error details");

		return -errno;
	}

	if (FAT_Read(filepath, &header, sizeof(header), 0, 0) < 0)
	{
		puts("Failed to read VFF header!");
		perror("Error details");

		return -errno;
	}

	if (!sanityCheckVFFHeader(&header, filesize))
	{
		puts("Bad VFF header!");

		return -EBADF;
	}

	if (!confirmation("Are you sure you want to restore your backup?", 6))
		return user_abort;

	return NANDRestoreFile(filepath, NAND_TARGET);
}

int delete()
{
	int ret;
	if ((ret = NAND_GetFileSize(NAND_TARGET, 0)) < 0)
	{
		printf("Failed to get file size**, did you delete it already? (%i)\n", ret);
		return ret;
	}

	if (!confirmation("Are you sure you want to delete your message board data??", 6))
		return user_abort;

	printf(">> Deleting %s from NAND... ", NAND_TARGET);
	ret = ISFS_Delete(NAND_TARGET);
	switch (ret)
	{
	case 0:
		puts("OK!");
		break;

	case -102:
		puts("\nTold you this was gonna happen... (Permission denied.)");
		break;

	case -106:
		puts("You already deleted it.");
		ret = 0;
		break;

	default:
		printf("Error! (%i)\n", ret);
		break;
	}

	return ret;
}

static int extract_cb(const char *path, FILINFO *st)
{
	char outpath[PATH_MAX];
	unsigned char buffer[0x10000];
	FIL fp[1] = {};
	FILE *out = NULL;
	int res;

	const char *ptr_path = strchr(path, ':');
	if (!ptr_path++)
		ptr_path = path;

	sprintf(outpath, "%s:%s%s", GetActiveDeviceName(), EXTRACT_PATH, ptr_path);
	puts(outpath);

	char *ptr = outpath;
	while ((ptr = strchr(ptr, '/')))
	{
		*ptr = 0;
		int ret = mkdir(outpath, 0644);

		if (ret < 0 && errno != EEXIST)
			goto error;

		*ptr++ = '/';
	}

	res = f_open(fp, path, FA_READ);
	if (res != FR_OK)
	{
		printf("%s: f_open() failed (%i)\n", path, res);
		return res;
	}

	out = fopen(outpath, "wb");
	if (!out)
	{
		f_close(fp);
		perror(outpath);
		return -errno;
	}

	UINT read = 0;
	while (true)
	{
		res = f_read(fp, buffer, sizeof buffer, &read);
		if (res || !read)
			break;

		if (!fwrite(buffer, read, 1, out))
		{
			res = -errno;
			break;
		}
	}

	f_close(fp);
	fclose(out);
	return res;

error:
	perror(outpath);
	return -errno;
}

#define MAX_PATH_LENGTH 256

void listFiles(const char *path)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
	{
		printf("Failed to open directory: %s\n", path);
		return;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		printf("%s", entry->d_name);

		// Skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		char full_path[256];
		snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

		if (entry->d_type == DT_DIR1)
		{
			// It's a directory, recurse into it
			listFiles(full_path);
		}
		else if (entry->d_type == DT_REG)
		{
			// It's a file, process it
			printf("Found file: %s\n", full_path);
			export_sd(full_path);
		}
	}

	closedir(dir);
}

bool is_ascii(const char *str)
{
	while (*str)
	{
		if (*str < 0 || *str > 127)
		{
			return false;
		}
		str++;
	}
	return true;
}

static int export_cb(const char *path, FILINFO *st)
{
	int res;

	uint32_t sendtime_x = 0;
	time_t sendtime, edittime;
	struct tm tm_sendtime = {};
	struct tm tm_edittime = {};

	FIL fp[1];
	struct CDBAttrHeader *cdbattr = NULL;
	struct CDBFILE *cdbfile = NULL;
	char outpath[PATH_MAX];
	FILE *text = NULL;

	if (!strcmp(st->fname, "cdb.conf"))
		return 0;

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

	char pathstr[60], timestr[60], editstr[60], datetimestr[60];

	edittime = SECONDS_TO_2000 + cdbattr->last_edit_time;
	gmtime_r(&edittime, &tm_edittime);
	strftime(datetimestr, sizeof(datetimestr), "%c", &tm_sendtime);
	strftime(editstr, sizeof(editstr), "%c", &tm_edittime);
	strftime(timestr, sizeof(timestr), "%F %r", &tm_sendtime);
	strftime(pathstr, sizeof(pathstr), "%Y/%m %b/%d/%H∶%M∶%S", &tm_sendtime);
	sprintf(outpath, "%s:%s/%s %s.txt", GetActiveDeviceName(), EXPORT_PATH, pathstr, cdbattr->description);

	printf("Processing: %s %s\n", timestr, cdbattr->description);

	char *ptr = outpath;
	while ((ptr = strchr(ptr, '/')))
	{
		*ptr = 0;
		int ret = mkdir(outpath, 0644);

		if (ret < 0 && errno != EEXIST)
			goto error;

		*ptr++ = '/';
	}

	text = fopen(outpath, "w");
	if (!text)
		goto error;

	utf16_t *ptr_desc = (void *)cdbfile + cdbfile->desc_offset;
	utf16_t *ptr_body = (void *)cdbfile + cdbfile->body_offset;
	size_t len_desc = 0;
	size_t len_body = 0;
	size_t len_desc8 = 0;
	size_t len_body8 = 0;

	while (ptr_desc[len_desc++])
		;
	while (ptr_body[len_body++])
		;

	len_desc8 = utf16_to_utf8(ptr_desc, len_desc, 0, 0);
	len_body8 = utf16_to_utf8(ptr_body, len_body, 0, 0);

	utf8_t *txt_buffer = malloc(len_body8 > len_desc8 ? len_body8 : len_desc8);
	if (!txt_buffer)
	{
		// Why are we still here
		fclose(text);
		goto error;
	}

	utf16_to_utf8(ptr_desc, len_desc, txt_buffer, len_desc8);
	fprintf(text, "Message title:  %s\n"
				  "Sender:         %016lld\n"
				  "Send date:      %s\n"
				  "Last edit date: %s\n"
				  "==================\n",
			(char *)txt_buffer, cdbfile->sender, datetimestr, editstr);

	utf16_to_utf8(ptr_body, len_body, txt_buffer, len_body8);
	fputs((char *)txt_buffer, text);

	free(txt_buffer);
	fclose(text);

	if (cdbfile->attachment_offset)
	{
		bool attachment_ajpg = false;
		const char *ext;
		uint32_t attachment_magic = (*(uint32_t *)((void *)cdbfile + cdbfile->attachment_offset));
		switch (attachment_magic)
		{
		case 0x55AA382D:
			ext = "U8";
			break;
		case 0x30335F30:
			ext = "ptm";
			break;
		case 0x414A5047:
			ext = "png";
			attachment_ajpg = true;
			sprintf(strrchr(outpath, '.'), "-attachment.%s", ext);
			decode((uint8_t *)cdbfile, cdbfile->attachment_size, cdbfile->attachment_offset, (char *)outpath);
			break;
		default:
			ext = "bin";
			break;
		}

		if (!attachment_ajpg)
		{
			text = fopen(outpath, "wb");
			if (!text)
				goto error;

			if (!fwrite((void *)cdbfile + cdbfile->attachment_offset, cdbfile->attachment_size, 1, text))
				goto error;
			fclose(text);
		}

		sprintf(strrchr(outpath, '.'), "-attachment.%s", ext);
		text = fopen(outpath, "wb");
		if (!text)
			goto error;

		if (!fwrite((void *)cdbfile + cdbfile->attachment_offset, cdbfile->attachment_size, 1, text))
			goto error;
		fclose(text);
	}

	goto finish;

error:
	res = -errno;
	perror(outpath);

finish:
	free(cdbattr);
	if (text)
		fclose(text);
	return res;
}

void export_sd(char *name)
{
	int res;

	FILE *pFile = fopen(name, "rb");
	if (!pFile)
	{
		perror("Failed to open file");
		goto error;
	}

	fseek(pFile, 0, SEEK_END);
	int fileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	uint8_t *buffer = (uint8_t *)malloc(fileSize);
	if (!buffer)
	{
		perror("Failed to allocate memory");
		fclose(pFile);
		goto error;
	}

	fread(buffer, 1, fileSize, pFile);
	fclose(pFile);

	uint8_t *attr = buffer;
	uint8_t *src = buffer + 0x400;

	struct CDBAttrHeader *cdbattr = (struct CDBAttrHeader *)attr;

	uint8_t key[16] = {0}; // Placeholder for the key
	struct AES_ctx str;
	AES_init_ctx_iv(&str, key, cdbattr->iv);
	AES_CBC_decrypt_buffer(&str, src, fileSize - 0x400);

	uint8_t *decryptedBuffer = (uint8_t *)malloc(fileSize);
	if (!decryptedBuffer)
	{
		perror("Failed to allocate memory for decryptedBuffer");
		free(buffer);
		goto error;
	}

	memcpy(decryptedBuffer, src, fileSize - 0x400);

	struct CDBFILE *cdbfile = (struct CDBFILE *)decryptedBuffer;

	uint32_t sendtime_x = 0;
	time_t sendtime;
	struct tm tm_sendtime = {};

	char *nameFile = strrchr(name, '/');

	if (!nameFile || !sscanf(nameFile + 1, "%X.000", &sendtime_x))
	{
		printf("%s: Invalid file name(?)\n", name);
		free(decryptedBuffer);
		free(buffer);
		goto error;
	}

	sendtime = SECONDS_TO_2000 + sendtime_x;
	gmtime_r(&sendtime, &tm_sendtime);

	uint8_t *buf = (uint8_t *)malloc(fileSize);
	if (buf == NULL)
	{
		fprintf(stderr, "Error: Failed to allocate memory.\n");
		free(decryptedBuffer);
		free(buffer);
		goto error;
	}

	memcpy(buf, attr, fileSize);

	if (!is_ascii(cdbattr->description))
	{
		free(buf);
		free(decryptedBuffer);
		free(buffer);
		goto error;
	}
	else
	{
		memmove(cdbattr->description + 1, cdbattr->description, strlen(cdbattr->description) + 1);
		cdbattr->description[0] = ' ';
	}

	char timestr[30];
	char datetimestr[60];
	char editstr[60];
	static const char mon[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	strftime(datetimestr, sizeof(datetimestr), "%F %r", &tm_sendtime);
	strftime(timestr, sizeof(timestr), "%p %I.%M.%S", &tm_sendtime);
	strftime(editstr, sizeof(editstr), "%c", &tm_sendtime);

	char outpath[PATH_MAX];
	sprintf(outpath, "%s:/cdb/%i/%s/%02i/%s %s.txt", GetActiveDeviceName(), tm_sendtime.tm_year + 1900, mon[tm_sendtime.tm_mon],
			tm_sendtime.tm_mday, timestr, cdbattr->description);

	printf("Processing SD: %s %s\n", datetimestr, cdbattr->description);

	char outpath1[PATH_MAX];
	sprintf(outpath1, "%s:/cdb", GetActiveDeviceName(), tm_sendtime.tm_year + 1900);
	mkdir(outpath1, 0644);
	sprintf(outpath1, "%s:/cdb/%i", GetActiveDeviceName(), tm_sendtime.tm_year + 1900);
	mkdir(outpath1, 0644);
	sprintf(outpath1, "%s:/cdb/%i/%s", GetActiveDeviceName(), tm_sendtime.tm_year + 1900, mon[tm_sendtime.tm_mon]);
	mkdir(outpath1, 0644);
	sprintf(outpath1, "%s:/cdb/%i/%s/%02i", GetActiveDeviceName(), tm_sendtime.tm_year + 1900, mon[tm_sendtime.tm_mon],
			tm_sendtime.tm_mday);
	mkdir(outpath1, 0644);

	FILE *text = fopen(outpath, "wb");

	if (!text)
	{
		perror("Failed to create output file");
		free(buf);
		free(decryptedBuffer);
		free(buffer);
		goto error;
	}

	utf16_t *ptr_desc = (void *)cdbfile + cdbfile->desc_offset;
	utf16_t *ptr_body = (void *)cdbfile + cdbfile->body_offset;
	size_t len_desc = 0;
	size_t len_body = 0;
	size_t len_desc8 = 0;
	size_t len_body8 = 0;

	while (ptr_desc[len_desc++])
		;
	while (ptr_body[len_body++])
		;

	len_desc8 = utf16_to_utf8(ptr_desc, len_desc, 0, 0);
	len_body8 = utf16_to_utf8(ptr_body, len_body, 0, 0);

	utf8_t *txt_buffer = malloc(len_body8 > len_desc8 ? len_body8 : len_desc8);
	if (!txt_buffer)
	{
		fclose(text);
		free(buf);
		free(decryptedBuffer);
		free(buffer);
		goto error;
	}

	utf16_to_utf8(ptr_desc, len_desc, txt_buffer, len_desc8);
	fprintf(text, "Message title:  %s\n"
				  "Sender:         %016lld\n"
				  "Send date:      %s\n"
				  "Last edit date: %s\n"
				  "==================\n",
			(char *)txt_buffer, cdbfile->sender, datetimestr, editstr);

	utf16_to_utf8(ptr_body, len_body, txt_buffer, len_body8);
	fputs((char *)txt_buffer, text);

	free(txt_buffer);
	fclose(text);

	if (cdbfile->attachment_offset)
	{
		bool attachment_ajpg = false;

		const char *ext;
		uint32_t attachment_magic = (*(uint32_t *)((void *)cdbfile + cdbfile->attachment_offset));
		switch (attachment_magic)
		{
		case 0x55AA382D:
			ext = "U8";
			break;
		case 0x30335F30:
			ext = "ptm";
			break;
		case 0x414A5047:
			ext = "png";
			attachment_ajpg = true;
			sprintf(strrchr(outpath, '.'), "-attachment.%s", ext);
			decode((uint8_t *)cdbfile, cdbfile->attachment_size, cdbfile->attachment_offset, (char *)outpath);
			break;
		default:
			ext = "bin";
			break;
		}

		if (!attachment_ajpg)
		{
			sprintf(strrchr(outpath, '.'), "-attachment.%s", ext);
			text = fopen(outpath, "wb");
			if (!text)
				goto error;

			if (!fwrite((void *)cdbfile + cdbfile->attachment_offset, cdbfile->attachment_size, 1, text))
				goto error;
			fclose(text);
		}
	}

error:
	res = -errno;
	perror(outpath);
	free(buf);
	free(decryptedBuffer);
	free(buffer);
	perror(outpath);

finish:
	perror(outpath);
	free(buf);
	free(decryptedBuffer);
	free(buffer);
	free(cdbattr);
	if (text)
		fclose(text);
}

static int copytree(const char *src, int (*callback)(const char *, FILINFO *))
{
	char path[PATH_MAX];

	FRESULT fres;
	DIR dp[1] = {};
	FILINFO st;

	fres = f_opendir(dp, src);
	if (fres != FR_OK)
	{
		printf("f_opendir failed! (%i)\n", fres);
		return fres;
	}

	while (((fres = f_readdir(dp, &st)) == FR_OK) && st.fname[0])
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

int extract(void)
{
	FATFS fs = {};

	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	FILE *fp = fopen(filepath, "rb");
	if (!fp)
	{
		perror(filepath);
		return -errno;
	}

	f_mount(&fs, "vff:", 0);

	int ret = VFFInit(fp, &fs);
	if (ret < 0)
		return ret;

	ret = copytree("vff:", extract_cb);
	if (!ret)
		puts("\nOK!");

	f_unmount("vff:");
	fclose(fp);
	return ret;
}

int export(void)
{
	FATFS fs = {};

	char filepath[PATH_MAX];
	sprintf(filepath, "%s:%s", GetActiveDeviceName(), FAT_TARGET);
	FILE *fp = fopen(filepath, "rb");
	if (!fp)
	{
		perror(filepath);
		return -errno;
	}

	listFiles("sd:/private/wii/title/HAEA/");

	f_mount(&fs, "vff:", 0);

	int ret = VFFInit(fp, &fs);
	if (ret < 0)
		return ret;

	ret = copytree("vff:", export_cb);
	if (!ret)
		puts("\nOK!");

	f_unmount("vff:");
	fclose(fp);
	return ret;
}

static MainMenuItem items[] =
	{
		{"Backup",
		 "\x1b[32;1m", // light green
		 backup},

		{"Restore",
		 "\x1b[31;1m", // light red
		 restore},

		{"Delete",
		 "\x1b[41;1m\x1b[30m", // Black on light red
		 delete},

		{"Extract (raw)",
		 "\x1b[33;0m",
		 extract},

		{"Export (text)",
		 "\x1b[33;1m", // light yellow
		 export},

		{"Exit",
		 "",
		 NULL}};

int main()
{
	bool ok = (patch_ahbprot_reset() && patch_isfs_permissions());
	initpads();
	ISFS_Initialize();

	if (!ok)
	{
		puts("Failed to patch NAND permissions, deleting is not going to work...");
		sleep(3);
	}

	if (!FATMount())
		goto waitexit;

	MainMenu(items, 6);

exit:
	FATUnmount();
	ISFS_Deinitialize();

	stoppads();
	return 0;

waitexit:
	printf("\nPress any button to return to loader.");
	while (!wait_button(0))
		;
	goto exit;
}
