#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "fatfs/ff.h"
#include "fatfs/diskio.h"

struct VFFHeader {
	uint32_t magic;			// 'VFF ' 0x56464620
	uint16_t BOM;			// 0xFEFF ("never checked"?)
	uint16_t length;		// Length of...... of what
	uint32_t fileSize;
	uint16_t clusterSize;	// 0x0020

	char padding[18];
};
static_assert(sizeof(struct VFFHeader) == 0x20, "Wrong size for VFF header!");

enum {
    VFF_SECTORSIZE = 0x200
};

bool sanityCheckVFFHeader(struct VFFHeader*, size_t filesize);
int VFFInit(FILE*, FATFS*);
void VFFShutdown(void);

DSTATUS vff_status(void);
DRESULT vff_read(void*, LBA_t, size_t);
DRESULT vff_write(const void*, LBA_t, size_t);
DRESULT vff_ioctl(int, void*);
