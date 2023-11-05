#include "fatMounter.h"

#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
#include <unistd.h>
#include <errno.h>

static const DISC_INTERFACE *sd_card = &__io_wiisd,
	*usb_msc = &__io_usbstorage;

static bool sd_mounted = false,
	usb_mounted = false;

bool mountSD() {
	if (sd_mounted) return sd_mounted;

	sd_card->startup();
	if (!sd_card->isInserted()) {
		sd_card->shutdown();
		errno = ENODEV;
		return false;
	}
	sd_mounted = fatMountSimple("sd", sd_card);
	chdir("sd:/");
	return sd_mounted;
}

void unmountSD() {
	if (sd_mounted) {
		fatUnmount("sd");
		sd_card->shutdown();
		sd_mounted = false;
	}
}

bool mountUSB() {
	if (usb_mounted) return usb_mounted;

	usb_msc->startup();
	if (!usb_msc->isInserted()) {
		usb_msc->shutdown();
		errno = ENODEV;
		return false;
	}

//	for(int r = 0; r < 10; r++) {
		usb_mounted = fatMountSimple("usb", usb_msc);
//		if(usb_mounted) break;
//	}
	chdir("usb:/");
	return usb_mounted;
}

void unmountUSB() {
	if (usb_mounted) {
		fatUnmount("usb");
		usb_msc->shutdown();
		usb_mounted = false;
	}
}

