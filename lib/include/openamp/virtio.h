/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * $FreeBSD$
 */

#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include <openamp/virtqueue.h>

#if defined __cplusplus
extern "C" {
#endif

/* VirtIO device IDs. */
#define VIRTIO_ID_NETWORK    0x01
#define VIRTIO_ID_BLOCK      0x02
#define VIRTIO_ID_CONSOLE    0x03
#define VIRTIO_ID_ENTROPY    0x04
#define VIRTIO_ID_BALLOON    0x05
#define VIRTIO_ID_IOMEMORY   0x06
#define VIRTIO_ID_RPMSG      0x07 /* virtio remote remote_proc messaging */
#define VIRTIO_ID_SCSI       0x08
#define VIRTIO_ID_9P         0x09

/* Status byte for guest to report progress. */
#define VIRTIO_CONFIG_STATUS_ACK       0x01
#define VIRTIO_CONFIG_STATUS_DRIVER    0x02
#define VIRTIO_CONFIG_STATUS_DRIVER_OK 0x04
#define VIRTIO_CONFIG_STATUS_NEEDS_RESET 0x40
#define VIRTIO_CONFIG_STATUS_FAILED    0x80

/*
 * Generate interrupt when the virtqueue ring is
 * completely used, even if we've suppressed them.
 */
#define VIRTIO_F_NOTIFY_ON_EMPTY (1 << 24)

/*
 * The guest should never negotiate this feature; it
 * is used to detect faulty drivers.
 */
#define VIRTIO_F_BAD_FEATURE (1 << 30)

/*
 * Some VirtIO feature bits (currently bits 28 through 31) are
 * reserved for the transport being used (eg. virtio_ring), the
 * rest are per-device feature bits.
 */
#define VIRTIO_TRANSPORT_F_START      28
#define VIRTIO_TRANSPORT_F_END        32

struct virtio_feature_desc {
	uint32_t vfd_val;
	const char *vfd_str;
};

/*
 * Structure definition for virtio devices for use by the
 * applications/drivers
 *
 */

struct virtio_device {
	/*
	 * Since there is no generic device structure so
	 * keep its type as void. The driver layer will take
	 * care of it.
	 */
	void *device;

	/* Device name */
	char *name;

	/* List of virtqueues encapsulated by virtio device. */
	//TODO : Need to implement a list service for ipc stack.
	void *vq_list;

	/* Virtio device specific features */
	uint32_t features;

	/*
	 * Pointer to hold some private data, useful
	 * in callbacks.
	 */
	void *data;
};

/* 
 * Helper functions.
 */
const char *virtio_dev_name(uint16_t devid);
void virtio_describe(struct virtio_device *dev, const char *msg,
		     uint32_t features,
		     struct virtio_feature_desc *feature_desc);

#if defined __cplusplus
}
#endif

#endif				/* _VIRTIO_H_ */
