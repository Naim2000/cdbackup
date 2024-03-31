#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "vff.h"
#include "fs.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

static FILE* s_vffFile = NULL;

bool sanityCheckVFFHeader(struct VFFHeader* hdr, size_t filesize) {
	return
		hdr->magic		 == 0x56464620
	&&	hdr->BOM		 == 0xFEFF
	&&	hdr->fileSize	 == filesize;
}

void VFFShutdown() { s_vffFile = NULL; }

int VFFInit(FILE* fp, FATFS* fs) {
    struct VFFHeader header[1] = {};

    if (!fread(header, sizeof(header), 1, fp)) {
        perror("Failed to read VFF header");
        return -errno;
    }

    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);

    if (!sanityCheckVFFHeader(header, filesize)) {
        puts("Bad VFF header!");
        return -EINVAL;
    }

    uint16_t clusterSize = header->clusterSize * 16;
    uint16_t clusterCount= header->fileSize / clusterSize;

    if (clusterCount < 0xFF5) {
        fs->fs_type = FS_FAT12;

        uint32_t tableSize = __align_up(((clusterCount + 1) / 2) * 3, clusterSize);
        fs->fsize = tableSize / clusterSize;
    }
    else if (clusterCount < 0xFFF5) {
        fs->fs_type = FS_FAT16;

        uint32_t tableSize = __align_up(clusterCount * 2, clusterSize);
        fs->fsize = tableSize / clusterSize;
    }
    else {
        printf("VFF is too large? (%hu * %hu)", clusterSize, clusterCount);
        return -E2BIG;
    }

    fs->n_fats = 2;
    fs->csize = 1;

    fs->n_rootdir = 0x80;

    uint32_t sysect = 1 + (fs->fsize * 2) + fs->n_rootdir / (0x200 / 0x20);
    uint32_t clusterCount_real = clusterCount - sysect;

    fs->n_fatent = clusterCount_real + 2;
    fs->volbase = 0;
    fs->fatbase = 1;
    fs->database = sysect;
    fs->dirbase = fs->fatbase + fs->fsize * 2;

    // fs->last_clst = fs->free_clst = 0xFFFFFFFF;
    fs->fsi_flag = 0x80;

    fs->id = 0;
    fs->cdir = 0;

    fs->wflag = 0;
    fs->winsect = (LBA_t)-1;

    s_vffFile = fp;

    return 0;
}

FRESULT vff_mount(FILE* fp, FATFS* fs) {
    fs->fs_type = 0;
    fs->pdrv = 0;

    if (VFFInit(fp, fs) < 0)
        return FR_DISK_ERR;

    return FR_OK;
}

DSTATUS vff_status(void) {
    DSTATUS res = 0;

    if (!s_vffFile) res |= STA_NODISK;

    return res;
}

DRESULT vff_read(void* buffer, LBA_t sec, size_t count) {
    if (!sec) {
        puts("vff: Tried to read sector 0!");
        return RES_PARERR;
    }

    off_t offset = sizeof(struct VFFHeader) + (--sec * VFF_SECTORSIZE);
    fseek(s_vffFile, offset, SEEK_SET);

    return fread(buffer, VFF_SECTORSIZE, count, s_vffFile) == count ? RES_OK : RES_ERROR;
}

DRESULT vff_write(const void* buffer, LBA_t sec, size_t count) {
    if (!sec) {
        puts("vff: Tried to write sector 0!");
        return RES_PARERR;
    }

    off_t offset = sizeof(struct VFFHeader) + (--sec * VFF_SECTORSIZE);
    fseek(s_vffFile, offset, SEEK_SET);

    return fwrite(buffer, VFF_SECTORSIZE, count, s_vffFile) == count ? RES_OK : RES_ERROR;
}

DRESULT vff_ioctl(int cmd, void* buf) {
    switch (cmd) {
        case CTRL_SYNC: {
            fflush(s_vffFile);
            return RES_OK;
        }

        case GET_SECTOR_COUNT: {
            fseek(s_vffFile, 0, SEEK_END);
            *(LBA_t*)buf = ftell(s_vffFile) / VFF_SECTORSIZE;
            return RES_OK;
        } 

        default: {
            printf("vff: unknown ioctl %i\n", cmd);
            return RES_OK;
        }
    }
}
