#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <gccore.h>

#include "fatMounter.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
static int conX = 0, conY = 0;

uint32_t wii_down = 0;
uint16_t gcn_down = 0;
uint16_t input_btns = 0;

void init_video() {
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	CON_Init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	printf("\x1b[%d;%dH", 2, 0);
	CON_GetMetrics(&conX, &conY);
}

void clearln() {
	putchar('\r');
	for (int i = 0; i < conX - 1; i++) putchar(' ');
	putchar('\r');
}

void quit(int ret) {
	FATUnmount();
	ISFS_Deinitialize();
	printf("\nPress HOME/START to return to loader.");
	input_scan();
	while(!input_pressed(input_home)) {
		input_scan();
		VIDEO_WaitVSync();
	}
	WPAD_Shutdown();
	exit(ret);
}

void input_scan(void) {
	input_btns = 0;

	WPAD_ScanPads();
	 PAD_ScanPads();
	wii_down = WPAD_ButtonsDown(0);
	gcn_down =  PAD_ButtonsDown(0);

	if (wii_down & WPAD_BUTTON_UP		|| gcn_down & PAD_BUTTON_UP)
		input_btns |= input_up;
	if (wii_down & WPAD_BUTTON_DOWN		|| gcn_down & PAD_BUTTON_DOWN)
		input_btns |= input_down;
	if (wii_down & WPAD_BUTTON_LEFT		|| gcn_down & PAD_BUTTON_LEFT)
		input_btns |= input_left;
	if (wii_down & WPAD_BUTTON_RIGHT	|| gcn_down & PAD_BUTTON_RIGHT)
		input_btns |= input_right;

	if (wii_down & WPAD_BUTTON_A		|| gcn_down & PAD_BUTTON_A)
		input_btns |= input_a;
	if (wii_down & WPAD_BUTTON_B		|| gcn_down & PAD_BUTTON_B)
		input_btns |= input_b;
	if (wii_down & WPAD_BUTTON_1		|| gcn_down & PAD_BUTTON_X)
		input_btns |= input_x;
	if (wii_down & WPAD_BUTTON_2		|| gcn_down & PAD_BUTTON_Y)
		input_btns |= input_y;

	if (wii_down & WPAD_BUTTON_PLUS		|| gcn_down & PAD_BUTTON_Y)
		input_btns |= input_start;
	if (wii_down & WPAD_BUTTON_MINUS	|| gcn_down & PAD_BUTTON_X)
		input_btns |= input_select;
	if (wii_down & WPAD_BUTTON_HOME		|| gcn_down & PAD_BUTTON_START || SYS_ResetButtonDown())
		input_btns |= input_home;
}


bool confirmation(const char* message, unsigned int wait_time) {
	printf(message);
	sleep(wait_time);
	printf("\n\nPress +/START to confirm.\nPress any other button to cancel.\n");

	input_scan();
	while (!input_pressed(~0) ) {
		input_scan();
		VIDEO_WaitVSync();
	}
	return input_pressed(input_start);
}

