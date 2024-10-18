#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>

#include "fatMounter.h"
#include "pad.h"
#include "video.h"

// Inspired by YAWM ModMii Edition
typedef struct {
	const char* friendlyName;
	const char* name;
	const DISC_INTERFACE* disk;
	int mounted;
} FATDevice;

static FATDevice devices[] = {
	{
		.friendlyName = "SD card slot",
		.name         = "sd",
		.disk         = &__io_wiisd
	},

	{
		.friendlyName = "USB mass storage device",
		.name         = "usb",
		.disk         = &__io_usbstorage
	},
};
#define NUM_DEVICES (sizeof(devices) / sizeof(FATDevice))

static FATDevice* active = NULL;

static void FATSetDefault(FATDevice* dev) {
	char driveroot[10];

	active = dev;
	sprintf(driveroot, "%s:/", dev->name);
	chdir(driveroot);
}

static bool FATMountDevice(FATDevice* dev) {
	printf ("[*]	Mounting '%s' ... ", dev->friendlyName);

	if ((dev->mounted = fatMountSimple(dev->name, dev->disk)))
		puts("OK!");
	else
		puts("Failed!");


	return dev->mounted;
}

bool FATMount() {
	FATDevice* attached[NUM_DEVICES] = {};
	int i = 0;

	// FATUnmount();
	for (FATDevice* dev = devices; dev < devices + NUM_DEVICES; dev++) {
		if (dev->mounted ||
			(dev->disk->startup() && dev->disk->isInserted() && FATMountDevice(dev))) {
		//	printf("[+]	Device detected:	\"%s\"\n", dev->friendlyName);
			attached[i++] = dev;
		}
	}

	if (i == 0) {
		puts("\x1b[30;1m[?]	No storage devices are attached.\x1b[39m");
		return false;
	}

	FATDevice* target = NULL;
	// if (i == 1) target = attached[0]; else
	{
		puts("[*]	Choose a device to use.");

		int index = 0;
		bool selected = false;
		while (!selected) {
			clearln();
			printf("[*]	Device: (%i/%i) %s", index + 1, i, attached[index]->friendlyName);

			switch (wait_button(0))
			{
				case WPAD_BUTTON_LEFT:
					if (index) index -= 1;
					break;

				case WPAD_BUTTON_RIGHT:
					if (++index == i) index = 0;
					break;

				case WPAD_BUTTON_A:
					target = attached[index];
				case WPAD_BUTTON_B:
				case WPAD_BUTTON_HOME:
					selected = true;
					break;


			}
		}
		clearln();
	}

	if (!target) return false;

	FATSetDefault(target);
	return true;
}

void FATUnmount() {
	for (FATDevice* dev = devices; dev < devices + NUM_DEVICES; dev++) {
		if (dev->mounted) {
			fatUnmount(dev->name);
			dev->disk->shutdown();
			dev->mounted = false;
		}
	}
	
	active = NULL;
}

const char* GetActiveDeviceName() { return active ? active->name : NULL; }

