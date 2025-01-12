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
#include <dirent.h>
#include "aes.h"
#include <setjmp.h>

#include "config.h"

#include "converter/converter.h"

void listFiles(const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp)))
    {
        char full_path[PATH_MAX];
        struct stat statbuf;

        // Ignore "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &statbuf) == -1)
        {
            perror("stat");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode))
        {
            // Recursively traverse subdirectory
            listFiles(full_path);
        }
        else if (S_ISREG(statbuf.st_mode))
        {
            // Check if the filename length is 12 bytes
            if (strlen(entry->d_name) == 12)
            {
                export_sd(full_path);
            }
        }
    }

    closedir(dp);
}
