/*
 * HIDAPI Linux stub backend (project-local).
 *
 * Minimal no-op implementation of the hidapi C API so the project links on
 * Linux without pulling in the full upstream linux/hid.c (hidraw + libudev).
 *
 * Effect: hid_init returns success, hid_enumerate returns NULL, all opens
 * fail. Higher-level controllers (StreamDeck, Xencelabs, SpaceMouse) will
 * therefore find no HID devices on Linux. macOS and Windows are unaffected
 * because HidApiPlatform.c only includes this file when __linux__ is set.
 *
 * To enable real HID device support on Linux, replace this stub by including
 * upstream hidapi linux/hid.c and linking against libudev.
 */

#include "../hidapi/hidapi.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct hid_device_ {
    int unused;
};

static const wchar_t * const kNoError = L"Success";
static const wchar_t * const kNotSupported =
    L"HIDAPI Linux backend is a stub: HID devices are not supported on this build";

int HID_API_EXPORT HID_API_CALL hid_init(void) {
    return 0;
}

int HID_API_EXPORT HID_API_CALL hid_exit(void) {
    return 0;
}

struct hid_device_info HID_API_EXPORT * HID_API_CALL
hid_enumerate(unsigned short vendor_id, unsigned short product_id) {
    (void) vendor_id;
    (void) product_id;
    return NULL;
}

void HID_API_EXPORT HID_API_CALL
hid_free_enumeration(struct hid_device_info *devs) {
    (void) devs;
}

HID_API_EXPORT hid_device * HID_API_CALL
hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number) {
    (void) vendor_id;
    (void) product_id;
    (void) serial_number;
    return NULL;
}

HID_API_EXPORT hid_device * HID_API_CALL
hid_open_path(const char *path) {
    (void) path;
    return NULL;
}

int HID_API_EXPORT HID_API_CALL
hid_write(hid_device *dev, const unsigned char *data, size_t length) {
    (void) dev;
    (void) data;
    (void) length;
    return -1;
}

int HID_API_EXPORT HID_API_CALL
hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds) {
    (void) dev;
    (void) data;
    (void) length;
    (void) milliseconds;
    return -1;
}

int HID_API_EXPORT HID_API_CALL
hid_read(hid_device *dev, unsigned char *data, size_t length) {
    (void) dev;
    (void) data;
    (void) length;
    return -1;
}

int HID_API_EXPORT HID_API_CALL
hid_set_nonblocking(hid_device *dev, int nonblock) {
    (void) dev;
    (void) nonblock;
    return -1;
}

int HID_API_EXPORT HID_API_CALL
hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length) {
    (void) dev;
    (void) data;
    (void) length;
    return -1;
}

int HID_API_EXPORT HID_API_CALL
hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length) {
    (void) dev;
    (void) data;
    (void) length;
    return -1;
}

int HID_API_EXPORT HID_API_CALL
hid_get_input_report(hid_device *dev, unsigned char *data, size_t length) {
    (void) dev;
    (void) data;
    (void) length;
    return -1;
}

void HID_API_EXPORT HID_API_CALL
hid_close(hid_device *dev) {
    (void) dev;
}

int HID_API_EXPORT_CALL
hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen) {
    (void) dev;
    if (string && maxlen > 0) string[0] = L'\0';
    return -1;
}

int HID_API_EXPORT_CALL
hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen) {
    (void) dev;
    if (string && maxlen > 0) string[0] = L'\0';
    return -1;
}

int HID_API_EXPORT_CALL
hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen) {
    (void) dev;
    if (string && maxlen > 0) string[0] = L'\0';
    return -1;
}

struct hid_device_info HID_API_EXPORT * HID_API_CALL
hid_get_device_info(hid_device *dev) {
    (void) dev;
    return NULL;
}

int HID_API_EXPORT_CALL
hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen) {
    (void) dev;
    (void) string_index;
    if (string && maxlen > 0) string[0] = L'\0';
    return -1;
}

int HID_API_EXPORT_CALL
hid_get_report_descriptor(hid_device *dev, unsigned char *buf, size_t buf_size) {
    (void) dev;
    (void) buf;
    (void) buf_size;
    return -1;
}

HID_API_EXPORT const wchar_t* HID_API_CALL
hid_error(hid_device *dev) {
    return dev ? kNotSupported : kNoError;
}

HID_API_EXPORT const struct hid_api_version* HID_API_CALL
hid_version(void) {
    static const struct hid_api_version v = {
        HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH
    };
    return &v;
}

HID_API_EXPORT const char* HID_API_CALL
hid_version_str(void) {
    return HID_API_VERSION_STR;
}
