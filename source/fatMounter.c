#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>

#include "fatMounter.h"
#include "tools.h"

// Inspired by YAWM ModMii Edition
typedef struct {
	const char* friendlyName;
	const char* name;
	const DISC_INTERFACE* disk;
	long mounted;
} FATDevice;

#define NUM_DEVICES 2
static FATDevice devices[NUM_DEVICES] = {
	{ "SD card slot",				"sd",	&__io_wiisd },
	{ "USB mass storage device",	"usb",	&__io_usbstorage},
};

static FATDevice* active = NULL;

bool FATMount() {
	FATDevice* attached[NUM_DEVICES] = {};
	int i = 0;

	for (int ii = 0; ii < NUM_DEVICES; ii++) {
		FATDevice* dev = devices + i;

		dev->disk->startup();
		if (dev->disk->isInserted()) {
			printf("[+]	Device detected:	\"%s\"\n", dev->friendlyName);
			attached[i++] = dev;
		}
		else dev->disk->shutdown();
	}

	if (i == 0) {
		puts("\x1b[30;1m[?]	No storage devices are attached.\x1b[39m");
		return false;
	}

	FATDevice* target = NULL;
	if (i == 1) target = attached[0];
	else {
		puts("[*]	Choose a device to mount.");

		int index = 0;
		bool selected = false;
		while (!selected) {
			clearln();
			printf("[*]	Device: < %s >", attached[index]->friendlyName);

			while (true) {
				input_scan();

				if 		(input_pressed(input_left))		{ if (index) index--; break; }
				else if (input_pressed(input_right))	{ if (++index == i) index = 0; break; }
				else if (input_pressed(input_a))		{ target = attached[index]; selected = true; break; }
				else if (input_pressed(input_home))		{ selected = true; break; }
			}
		}
		clearln();
	}

	if (!target) return false;

	printf ("[*]	Mounting %s:/ ... ", target->name);
	if (fatMountSimple(target->name, target->disk)) {
		printf("OK!\n\n");
		target->mounted = true;
		active = target;
	}
	else {
		printf("Failed!\n\n");
		target->mounted = false;
		target->disk->shutdown();
	}
	
	return target->mounted;
}

void FATUnmount() {
	for (int i = 0; i < NUM_DEVICES; i++) {
		FATDevice* dev = devices + i;

		if (dev->mounted) {
			fatUnmount(dev->name);
			dev->disk->shutdown();
			dev->mounted = false;
		}
	}
	
	active = NULL;
}

const char* GetActiveDeviceName() { return active? active->name : NULL; }

