/* https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/IOS/Network/KD/VFF/VFFUtil.cpp */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "vff.h"
#include "fs.h"

bool sanityCheckVFFHeader(struct VFFHeader* hdr, size_t filesize) {
	return
		hdr->magic		 == VFF_MAGIC
	&&	hdr->endian		 == VFF_LITTLE_ENDIAN
	&&	hdr->file_size	 == filesize;
}

int VFF_ReadClusters(VFFHandle* vff, unsigned offset, unsigned count, void* out) {
    if (!vff || !out)
        return 0;

    if (fseek(vff->fp, sizeof(struct VFFHeader) + (offset * vff->cluster_size), SEEK_SET) < 0) {
        perror("fseek");
        return 0;
    }

    return fread(out, vff->cluster_size, count, vff->fp);
}

int VFF_Init(const char* filepath, VFFHandle* vff) {
    FILE* fp;
    struct VFFHeader header[1] = {};

    fp = fopen(filepath, "rb");
    if (!fp) {
        perror("fopen");
        return -errno;
    }

    if (!fread(header, sizeof(header), 1, fp)) {
        perror("Failed to read VFF header");
        fclose(fp);
        return -errno;
    }

    // fseek(fp, 0, SEEK_END);
    // size_t filesize = ftell(fp);

    unsigned cluster_size = vff->cluster_size = header->cluster_size << 4; // ??????
    unsigned cluster_count = header->file_size / cluster_size;

    if (cluster_count < FAT12_MAX) {
        vff->fat_type = VFF_TYPE_FAT12;
        vff->fat_size = __align_up(((cluster_count + 1) / 2) * 3, cluster_size) / cluster_size;
    }
    else if (cluster_count < FAT16_MAX) {
        vff->fat_type = VFF_TYPE_FAT16;
        vff->fat_size = __align_up(cluster_count * 2, cluster_size) / cluster_size;
    }
    else {
        printf("VFF is too large? (%hu * %hu)", cluster_size, cluster_count);
        fclose(fp);
        return -E2BIG;
    }

    vff->fp            = fp;
    vff->rootdir_start = (vff->fat_size * 2);
    vff->rootdir_count = 128;
    vff->data_start    = vff->rootdir_start + ((vff->rootdir_count * sizeof(FATDirEnt)) / cluster_size);
    vff->fat_count     = cluster_count - vff->data_start;

    vff->fats[0] = calloc(vff->fat_size, vff->cluster_size);
    vff->fats[1] = calloc(vff->fat_size, vff->cluster_size);

    VFF_ReadClusters(vff, 0, vff->fat_size, vff->fats[0]);
    VFF_ReadClusters(vff, vff->fat_size, vff->fat_size, vff->fats[1]);

    return 0;
}

void VFF_Close(VFFHandle* vff) {
    if (!vff)
        return;

    free(vff->fats[0]);
    free(vff->fats[1]);
    fclose(vff->fp);
    vff->fp = NULL;
}

