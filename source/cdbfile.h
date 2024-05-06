/*
 * Observations from giantpune's mailboxbomb code.
 */

#define SECONDS_TO_2000		946684800ULL

struct CDBFILE
{
	/* 0x000 */ char magic[4]; // 0x52495f35 'RI_5'
	/* 0x004 */ float pos_x, pos_y; // are these floats?
	/* 0x00c */ uint32_t type;
	/* 0x010 */ uint64_t send_time; // in ticks!?
	/* 0x018 */ char sender[0x100]; // ?
	/* 0x118 */ uint32_t type2;
	/* 0x11C */ uint32_t desc_offset;
	/* 0x120 */ uint32_t body_offset;
	/* 0x124 */ uint32_t unk1;
	/* 0x128 */ uint32_t attachment_type;
	/* 0x12C */ uint32_t attachment_offset;
	/* 0x130 */ uint32_t attachment_size;
	/* 0x134 */ uint32_t unk3;
	/* 0x138 */ uint32_t unk4;
	/* 0x13c */ uint32_t unk5;
	/* 0x140 */ uint32_t crc32; // CRC32 of this entire header (0x140 bytes)
};

struct CDBAttrHeader
{
	/* 0x00 */ char magic[7];	// "CDBFILE"
	/* 0x07 */ uint8_t version;	// ? like it's 0x02

	/*
	 * How to calculate the wii ID:
	 * 1. Get Wii's MAC address.
	 * 	{ 0x00, 0x1B, 0xEA, 0xAA, 0xBB, 0xCC }
	 *
	 * 2. Append { 0x75, 0x79, 0x79 }.
	 * 	{ 0x00, 0x1B, 0xEA, 0xAA, 0xBB, 0xCC, 0x75, 0x79, 0x79 }
	 *
	 * 3. Calculate SHA-1.
	 *
	 * 	  vvvvvvvvvv  vvvvvvvvvv That's your wii ID!
	 * 	{ 0x4ff726c6, 0xe14b91a4, 0x1d128dd4, 0xa56daabe, 0xb75dd691 }
	 * 	  That's the HMAC key!    ^^^^^^^^^^  ^^^^^^^^^^  ^^^^^^^^^^
	 */
	/* 0x08 */ uint64_t wii_id;
	/* 0x10 */ uint8_t description_len;	// Length of description string incl. null byte
	/* 0x11 */ char unk1[3];
	/* 0x14 */ char description[0x70 - 0x14];

	/* 0x70 */ uint32_t entry_id;		// "/cdb.conf value".
	/* 0x74 */ uint32_t edit_count;
	/* 0x78 */ uint32_t file_size;		// {1}
	/* 0x7c */ uint32_t last_edit_time;	// This is also the file name (in hex)
	/* 0x80 */ char keystring[0x20];	// ? {1}
	/* 0xA0 */ unsigned char iv[16];	// {1} {2}
	/* 0xB0 */ unsigned char sha1_hmac[20];
	/* 0xC4 */ unsigned char padding[0x400 - 0xC4]; // or unknown. just using it as filler space

	/* 0x400 */ struct CDBFILE cdbfile[];
};
_Static_assert(sizeof(struct CDBAttrHeader) == 0x400, "Fix the padding");

/*
 * {1}: "Messages stored on the SD card (as opposed to the Wii's NAND) are signed and encrypted. These values are only present in encrypted messages."
 *
 * {2}: The key is all zeroes it seems.
 */


