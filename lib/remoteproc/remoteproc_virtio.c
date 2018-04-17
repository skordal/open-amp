/*
 * Remoteproc Virtio Framwork Implementation
 *
 * Copyright(c) 2018 Xilinx Ltd.
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name Texas Instruments nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <openamp/remoteproc.h>
#include <openamp/remoteproc_virtio.h>
#include <openamp/virtqueue.h>
#include <metal/utilities.h>
#include <metal/alloc.h>

static void rproc_virtio_virtqueue_notify(struct virtqueue *vq)
{
	struct virtio_device *vdev;
	struct remoteproc_virtio *rpvdev;
	struct virtio_vring *rvring;
	unsigned int vq_id = vq->vq_queue_index;

	vdev = vq->vq_dev;
	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	assert(vq_id <= vdev->num_vrings);
	rvring = &vdev->rvrings[vq_id];
	rpvdev->notify(rpvdev->priv, rvring->notifyid);
}

static int rproc_virtio_create_virtqueues(struct virtio_device *vdev,
					  unsigned int flags,
					  unsigned int nvqs,
					  const char *names[],
					  vq_callback *callbacks[],
					  struct virtqueue *vqs[])
{
	struct virtio_vring *rvring;
	struct vring_alloc_info vring_info;
	unsigned int num_vrings, i;
	int ret;
	(void)flags;

	num_vrings = vdev->vrings_num;
	if (nvqs > num_vrings)
		return -RPROC_EINVAL;
	/* Initialize virtqueue for each vring */
	for (i = 0; i < nvqs; i++) {
		rvring = &vdev->rvrings[i];
		if (vdev->role == VIRTIO_DEV_HOST) {
			size_t offset;
			struct metal_io_region *io = rvring->io;
			unsigned int num_descs = rvring->num_descs;
			unsigned int align = rvring->align;

			vring_info.vaddr = rvring->va;
			vring_info.align = rvring->align;
			vring_info.num_descs = rvring->num_descs;
			offset = metal_io_virt_to_offset(io, rvring->va);
			metal_io_block_set(io, offset, 0,
					   vring_size(num_descs, align));
		}
		ret = virtqueue_create(vdev, i,
					names[i], &vring_info,
					callbacks[i],
					rproc_virtio_virtqueue_notify,
					rvring->vq);
		if (ret)
			return -RPROC_EINVAL;
	}
	return 0;
}

static unsigned char rproc_virtio_get_status(struct virtio_device *vdev)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;
	char status;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	status = metal_io_read8(io,
				metal_io_virt_to_offset(io, &vdev_rsc->status));
	return status;
}

static void rproc_virtio_set_status(struct virtio_device *vdev,
				    unsigned char status)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	metal_io_write8(io,
			metal_io_virt_to_offset(io, &vdev_rsc->status),
			status);
}

static uint32_t rproc_virtio_get_features(struct virtio_device *vdev)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;
	uint32_t features;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	/* TODO: shall we get features based on the role ? */
	features = metal_io_read32(io,
				  metal_io_virt_to_offset(io,
				  &vdev_rsc->dfeatures));
	return features;
}

static void rproc_virtio_set_features(struct virtio_device *vdev,
				      uint32_t features)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	/* TODO: shall we set features based on the role ? */
	metal_io_write32(io,
			 metal_io_virt_to_offset(io, &vdev_rsc->status),
			 features);
}

static uint32_t rproc_virtio_negotiate_features(struct virtio_device *vdev,
						uint32_t features)
{
	(void)vdev;
	(void)features;

	return 0;
}

static void rproc_virtio_read_config(struct virtio_device *vdev,
				     uint32_t offset, void *dst, int length)
{
	(void)vdev;
	(void)offset;
	(void)dst;
	(void)length;
}

static void rpmsg_virtio_write_config(struct virtio_device *vdev,
				      uint32_t offset, void *src, int length)
{
	(void)vdev;
	(void)offset;
	(void)src;
	(void)length;
}

static void rproc_virtio_reset_device(struct virtio_device *vdev)
{
	if (vdev->role == VIRTIO_DEV_GUEST)
		rproc_virtio_set_status(vdev,
					VIRTIO_CONFIG_STATUS_NEEDS_RESET);
}

virtio_dispatch remoteproc_virtio_dispatch_funcs = {
	.create_virtqueues = rproc_virtio_create_virtqueues,
	.get_status =  rproc_virtio_get_status,
	.set_status = rproc_virtio_set_status,
	.get_features = rproc_virtio_get_features,
	.set_features = rproc_virtio_set_features,
	.negotiate_features = rproc_virtio_negotiate_features,
	.read_config = rproc_virtio_read_config,
	.write_config = rpmsg_virtio_write_config,
	.reset_device = rproc_virtio_reset_device,
};

struct virtio_device *
rproc_virtio_create_vdev(unsigned int role, unsigned int notifyid,
			 void *rsc, struct metal_io_region *rsc_io,
			 void *priv,
			 rpvdev_notify_func notify,
			 virtio_dev_reset_cb rst_cb)
{
	struct remoteproc_virtio *rpvdev;
	struct virtio_vring *rvrings;
	struct fw_rsc_vdev *vdev_rsc = rsc;
	struct virtio_device *vdev;
	unsigned int num_vrings = vdev_rsc->num_of_vrings;
	unsigned int i;

	rpvdev = metal_allocate_memory(sizeof(*rpvdev));
	if (!rpvdev)
		return NULL;
	rvrings = metal_allocate_memory(sizeof(*rvrings) * num_vrings);
	if (!rvrings)
		goto err0;
	memset(rpvdev, 0, sizeof(*rpvdev));
	memset(rvrings, 0, sizeof(*rvrings));
	vdev = &rpvdev->vdev;

	for (i = 0; i < num_vrings; i++) {
		struct virtqueue *vq;
		struct fw_rsc_vdev_vring *vring_rsc;

		vring_rsc = &vdev_rsc->vring[i];
		vq = virtqueue_allocate(vring_rsc->num);
		if (!vq)
			goto err1;
		rvrings[i].vq = vq;
	}

	/* FIXME commended as seems not nedded, already stored in vdev */
	//rpvdev->notifyid = notifyid;
	rpvdev->notify = notify;
	rpvdev->priv = priv;
	vdev->rvrings = rvrings;
	/* Assuming the shared memory has been mapped and registered if
	 * necessary
	 */
	rpvdev->vdev_rsc = vdev_rsc;
	rpvdev->vdev_rsc_io = rsc_io;

	vdev->index = notifyid;
	vdev->role = role;
	vdev->reset_cb = rst_cb;
	vdev->vrings_num = num_vrings;
	metal_spinlock_init(&vdev->lock);
	/* TODO: Shall we set features here ? */

	return &rpvdev->vdev;

err1:
	for (i = 0; i < num_vrings; i++) {
		if (rvrings[i].vq)
			metal_free_memory(rvrings[i].vq);
	}
	metal_free_memory(rvrings);
err0:
	metal_free_memory(rpvdev);
	return NULL;
}

int rproc_virtio_init_vring(struct virtio_device *vdev, unsigned int index,
			    unsigned int notifyid, void *va,
			    struct metal_io_region *io,
			    unsigned int num_descs, unsigned int align)
{
	struct virtio_vring *rvring;
	unsigned int num_vrings;;

	num_vrings = vdev->vrings_num;
	if (index >= num_vrings)
		return -RPROC_EINVAL;
	rvring = &vdev->rvrings[index];
	rvring->va = va;
	rvring->io = io;
	rvring->notifyid = notifyid;
	rvring->num_descs = num_descs;
	rvring->align = align;
}

int rproc_virtio_set_shm(struct remoteproc_virtio *rpvdev,
			 struct shm_pool *shm)
{
	if (!rpvdev)
		return -RPROC_EINVAL;
	rpvdev->shm = shm;

	return 0;
}
