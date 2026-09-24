/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 * Echoes every rpmsg message Linux sends to the "beagley-ipc-echo" service
 * back to its sender, on the C7x DSP of a BeagleY-AI.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

#include <openamp/open_amp.h>
#include <metal/device.h>
#include <resource_table.h>
#include <soc.h>

LOG_MODULE_REGISTER(ipc_echo, LOG_LEVEL_INF);

#define SERVICE_NAME "beagley-ipc-echo"

#define SHM_NODE       DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE       DT_REG_SIZE(SHM_NODE)

static const struct device *const ipm_handle = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

static metal_phys_addr_t shm_physmap = SHM_START_ADDR;
static metal_phys_addr_t rsc_physmap;
static struct metal_io_region shm_io_data;
static struct metal_io_region rsc_io_data;
static struct rpmsg_virtio_device rvdev;
static struct rpmsg_endpoint ep;
static void *rsc_table;

static K_SEM_DEFINE(rx_sem, 0, 1);

static void ipm_callback(const struct device *dev, void *context, uint32_t id, volatile void *data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(id);

	/* answered here, as this interrupt still runs after a fault has aborted main */
	if (data != NULL && *(volatile uint32_t *)data == TI_K3_RP_MBOX_SHUTDOWN) {
		uint32_t ack = TI_K3_RP_MBOX_SHUTDOWN_ACK;

		LOG_INF("shutdown requested");
		ipm_send(dev, 0, 0, &ack, sizeof(ack));
		sys_poweroff();
	}
	k_sem_give(&rx_sem);
}

static int mailbox_notify(void *priv, uint32_t id)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(id);

	ipm_send(ipm_handle, 0, 0, NULL, 0);
	return 0;
}

static int echo_cb(struct rpmsg_endpoint *endpoint, void *data, size_t len, uint32_t src,
		   void *priv)
{
	ARG_UNUSED(src);
	ARG_UNUSED(priv);

	LOG_INF("echoing %u bytes", (unsigned int)len);
	rpmsg_send(endpoint, data, len);

	return RPMSG_SUCCESS;
}

static void unbind_cb(struct rpmsg_endpoint *endpoint)
{
	rpmsg_destroy_ept(endpoint);
}

int main(void)
{
	struct metal_init_params params = METAL_INIT_DEFAULTS;
	struct fw_rsc_vdev_vring *vring;
	struct virtio_device *vdev;
	int rsc_size;
	int ret;

	LOG_INF("BeagleY-AI C7x IPC echo starting");

	ret = metal_init(&params);
	if (ret) {
		LOG_ERR("metal_init: %d", ret);
		return ret;
	}

	metal_io_init(&shm_io_data, (void *)SHM_START_ADDR, &shm_physmap, SHM_SIZE, -1, 0, NULL);

	rsc_table_get(&rsc_table, &rsc_size);
	rsc_physmap = (uintptr_t)rsc_table;
	metal_io_init(&rsc_io_data, rsc_table, &rsc_physmap, rsc_size, -1, 0, NULL);

	if (!device_is_ready(ipm_handle)) {
		LOG_ERR("IPM device not ready");
		return -ENODEV;
	}

	ipm_register_callback(ipm_handle, ipm_callback, NULL);

	ret = ipm_set_enabled(ipm_handle, 1);
	if (ret) {
		LOG_ERR("ipm_set_enabled: %d", ret);
		return ret;
	}

	vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE, VDEV_ID, rsc_table_to_vdev(rsc_table),
					&rsc_io_data, NULL, mailbox_notify, NULL);
	if (!vdev) {
		LOG_ERR("rproc_virtio_create_vdev failed");
		return -ENODEV;
	}

	/* Linux allocates the vrings, so wait until it has published them. */
	rproc_virtio_wait_remote_ready(vdev);

	vring = rsc_table_get_vring0(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring->notifyid, (void *)vring->da, &rsc_io_data,
				      vring->num, vring->align);
	if (ret) {
		LOG_ERR("init_vring 0: %d", ret);
		return ret;
	}

	vring = rsc_table_get_vring1(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring->notifyid, (void *)vring->da, &rsc_io_data,
				      vring->num, vring->align);
	if (ret) {
		LOG_ERR("init_vring 1: %d", ret);
		return ret;
	}

	ret = rpmsg_init_vdev(&rvdev, vdev, NULL, &shm_io_data, NULL);
	if (ret) {
		LOG_ERR("rpmsg_init_vdev: %d", ret);
		return ret;
	}

	ret = rpmsg_create_ept(&ep, rpmsg_virtio_get_rpmsg_device(&rvdev), SERVICE_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, echo_cb, unbind_cb);
	if (ret) {
		LOG_ERR("rpmsg_create_ept: %d", ret);
		return ret;
	}

	LOG_INF("announced \"%s\", waiting for messages", SERVICE_NAME);

	for (;;) {
		k_sem_take(&rx_sem, K_FOREVER);
		rproc_virtio_notified(vdev, VRING1_ID);
	}
}
