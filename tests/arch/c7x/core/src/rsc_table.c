/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <resource_table.h>

/* The remoteproc loader needs the resource table, which also holds the console trace buffer */
static int rsc_table_link(void)
{
	void *rsc;
	int len;

	rsc_table_get(&rsc, &len);
	return 0;
}

SYS_INIT(rsc_table_link, POST_KERNEL, 0);
