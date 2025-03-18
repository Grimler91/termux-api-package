#include <stdio.h>

#include "termux-usb.h"

int main() {
	struct termux_usb_device **devices;
	struct termux_usb_device *device;
	ssize_t num_devs;
	int i = 0;

	num_devs = termux_usb_get_device_list(&devices);
	if (num_devs < 0)
		return 0;
	while ((device = devices[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		struct termux_usb_config_descriptor *conf_desc = NULL;
		int j;
		printf("device %s\n", device->device_address);

		termux_usb_get_config_descriptor(device, 0, &conf_desc);
		if (conf_desc) {
			for (j = 0; j < conf_desc->bNumInterfaces; j++) {
				const struct termux_usb_interface_descriptor *intf_desc = &conf_desc->interface[j];
				if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
					printf("Found HID device\n");
				}
			}
		}
		termux_usb_free_config_descriptor(conf_desc);
	}
	termux_usb_free_device_list(devices);
}
