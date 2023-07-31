/*-------------------------------------------------------------
 
id.c -- ES Identification code
 
Copyright (C) 2008 tona
Unless other credit specified
 
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.
 
Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:
 
1.The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
 
2.Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.
 
3.This notice may not be removed or altered from any source
distribution.
 
-------------------------------------------------------------*/

#include <stdio.h>
#include <gccore.h>

#include "wiibasics.h"
#include "id.h"
#include "certs_dat.h"
#include "su_tmd_dat.h"
#include "su_tik_dat.h"

/* Debug functions adapted from libogc's es.c */
//#define DEBUG_ES
//#define DEBUG_IDENT
#define ISALIGNED(x) ((((u32)x)&0x1F)==0)

#ifdef DEBUG_IDENT
s32 __sanity_check_certlist(const signed_blob *certs, u32 certsize)
{
	int count = 0;
	signed_blob *end;
	
	if(!certs || !certsize) return 0;
	
	end = (signed_blob*)(((u8*)certs) + certsize);
	while(certs != end) {
#ifdef DEBUG_ES
		printf("Checking certificate at %p\n",certs);
#endif
		certs = ES_NextCert(certs);
		if(!certs) return 0;
		count++;
	}
#ifdef DEBUG_ES
	printf("Num of certificates: %d\n",count);
#endif
	return count;
}
#endif

s32 Identify(const u8 *certs, u32 certs_size, const u8 *idtmd, u32 idtmd_size, const u8 *idticket, u32 idticket_size) {
	s32 ret;
	u32 keyid = 0;
	ret = ES_Identify((signed_blob*)certs, certs_size, (signed_blob*)idtmd, idtmd_size, (signed_blob*)idticket, idticket_size, &keyid);
#ifdef DEBUG_IDENT
		printf("\tTicket: %u Std: %u Max: %u\n", idticket_size, STD_SIGNED_TIK_SIZE, MAX_SIGNED_TMD_SIZE);
		printf("\tCerts: Sane? %d\n", __sanity_check_certlist((signed_blob*)certs, certs_size));
		if (!ISALIGNED(certs)) printf("\tCertificate data is not aligned!\n");
		if (!ISALIGNED(idtmd)) printf("\tTMD data is not aligned!\n");
		if (!ISALIGNED(idticket)) printf("\tTicket data is not aligned!\n");
#endif
	return ret;
}



s32 Identify_SU(void) {
	fflush(stdout);
	return Identify(certs_dat, certs_dat_size, su_tmd_dat, su_tmd_dat_size, su_tik_dat, su_tik_dat_size);
}

s32 Identify_SysMenu(void) {
	s32 ret;
	u32 sysmenu_tmd_size, sysmenu_ticket_size;
	static u8 sysmenu_tmd[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	static u8 sysmenu_ticket[STD_SIGNED_TIK_SIZE] ATTRIBUTE_ALIGN(32);
	
	printf("Reading SM TMD...\n");
	ret = ISFS_ReadFileToArray ("/title/00000001/00000002/content/title.tmd", sysmenu_tmd, MAX_SIGNED_TMD_SIZE, &sysmenu_tmd_size);
	if (ret < 0) {
		printf("failed! %d\n", ret);
		return ret;
	}
	
	printf("\n\tPulling Sysmenu Ticket...");
	ret = ISFS_ReadFileToArray ("/ticket/00000001/00000002.tik", sysmenu_ticket, STD_SIGNED_TIK_SIZE, &sysmenu_ticket_size);
	if (ret < 0) {
		printf("failed! %d\n", ret);
		return ret;
	}
	
	fflush(stdout);
	return Identify(certs_dat, certs_dat_size, sysmenu_tmd, sysmenu_tmd_size, sysmenu_ticket, sysmenu_ticket_size);
}
