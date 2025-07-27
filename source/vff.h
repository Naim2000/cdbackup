#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef struct VFFHeader {
	uint32_t magic;
	uint16_t endian;
	uint16_t unk;
	uint32_t file_size;
	uint16_t cluster_size;
	uint16_t empty;
	uint16_t unknown;

	char padding[14];
} VFFHeader;
static_assert(sizeof(struct VFFHeader) == 0x20, "Wrong size for VFF header!");

// https://wiki.osdev.org/FAT#Directories_on_FAT12/16/32

typedef struct FATDirEnt {
	union {
		char name[11];
		struct {
			char basename[8];
			char ext[3];
		};
	};
	union {
		uint8_t attributes;
		struct {
			uint8_t : 2;
			uint8_t archive: 1;
			uint8_t directory: 1;
			uint8_t volume_id: 1; // label* ?
			uint8_t system: 1;
			uint8_t hidden: 1;
			uint8_t read_only: 1;
		};
	};
	uint8_t reserved;
	uint8_t dont_care;
	uint16_t ctime[2];
	uint16_t atime;
	uint16_t fat_hi; // zero
	uint16_t mtime[2];
	uint16_t fat_lo;
	uint32_t filesize;
} __attribute__((packed)) FATDirEnt;
static_assert(sizeof(FATDirEnt) == 0x20, "?");

enum {
	VFF_MAGIC = 0x56464620,

	/* This is probably in respect to the FAT itself and not the header;
	 * I believe the idea is that the 16 bit integer will read as 0xFFFE if the host's endian is the same as the file's. */
	VFF_BIG_ENDIAN = 0xFFFE,
	VFF_LITTLE_ENDIAN = 0xFEFF,

	VFF_TYPE_NONE = 0,
	VFF_TYPE_FAT12 = 1,
	VFF_TYPE_FAT16 = 2,

	FAT12_MAX = 0xFF5,
	FAT16_MAX = 0xFFF5,

	VFF_SECTOR_SIZE = 0x200,
};

typedef struct VFFHandle {
	FILE    *fp;

	int      fat_type;
	unsigned cluster_size;
	unsigned fat_size; // clusters

	unsigned rootdir_start;
	unsigned rootdir_count;

	unsigned data_start;
	unsigned fat_count;

	void     *fats[2];
} VFFHandle;

bool sanityCheckVFFHeader(struct VFFHeader *, size_t filesize);
int  VFF_Init(const char *, VFFHandle *);
void VFF_Close(VFFHandle *);
