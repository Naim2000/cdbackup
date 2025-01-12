#include <stdio.h>
#include <stdlib.h>
#include <ogc/system.h>
#include <ogc/cache.h>
#include <ogc/video.h>
#include <ogc/color.h>
#include <ogc/consol.h>

#include "video.h"
#include "malloc.h"

static void *xfb = NULL;
static GXRModeObj vmode = {};
static int conX, conY;

static GXRModeObj *rmode = NULL;

// from LoadPriiloader
__attribute__((constructor)) void init_video()
{
	// Initialise the video system
	VIDEO_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
	// SYS_STDIO_Report(true);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(false);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}

void clear()
{
	VIDEO_WaitVSync();
	//	VIDEO_ClearFrameBuffer(&vmode, xfb, COLOR_BLACK);
	printf("%s", "\x1b[2J");
}

void clearln()
{
	putchar('\r');
	for (int i = 1; i < conX; i++)
		putchar(' ');

	putchar('\r');
	fflush(stdout);
}
