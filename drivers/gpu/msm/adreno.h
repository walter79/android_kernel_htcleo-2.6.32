/* Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __ADRENO_H
#define __ADRENO_H

#include "kgsl_device.h"
#include "adreno_drawctxt.h"
#include "adreno_ringbuffer.h"

#define DEVICE_3D_NAME "kgsl-3d"
#define DEVICE_3D0_NAME "kgsl-3d0"

#define ADRENO_DEVICE(device) \
		KGSL_CONTAINER_OF(device, struct adreno_device, dev)

/* Flags to control command packet settings */
#define KGSL_CMD_FLAGS_NONE             0x00000000
#define KGSL_CMD_FLAGS_PMODE		0x00000001
#define KGSL_CMD_FLAGS_NO_TS_CMP	0x00000002
#define KGSL_CMD_FLAGS_NOT_KERNEL_CMD	0x00000004

/* Command identifiers */
#define KGSL_CONTEXT_TO_MEM_IDENTIFIER	0x2EADBEEF
#define KGSL_CMD_IDENTIFIER		0x2EEDFACE
#define KGSL_START_OF_IB_IDENTIFIER	0x2EADEABE
#define KGSL_END_OF_IB_IDENTIFIER	0x2ABEDEAD

#ifdef CONFIG_MSM_SCM
#define ADRENO_DEFAULT_PWRSCALE_POLICY  (&kgsl_pwrscale_policy_tz)
#else
#define ADRENO_DEFAULT_PWRSCALE_POLICY  NULL
#endif

#define ADRENO_ISTORE_START 0x5000 /* Istore offset */

enum adreno_gpurev {
	ADRENO_REV_UNKNOWN = 0,
	ADRENO_REV_A200 = 200,
	ADRENO_REV_A203 = 203,
	ADRENO_REV_A205 = 205,
	ADRENO_REV_A220 = 220,
	ADRENO_REV_A225 = 225,
	ADRENO_REV_A305 = 305,
	ADRENO_REV_A320 = 320,
};

struct adreno_gpudev;

struct adreno_device {
	struct kgsl_device dev;    /* Must be first field in this struct */
	unsigned int chip_id;
	enum adreno_gpurev gpurev;
	struct kgsl_memregion gmemspace;
	struct adreno_context *drawctxt_active;
	const char *pfp_fwfile;
	unsigned int *pfp_fw;
	size_t pfp_fw_size;
	const char *pm4_fwfile;
	unsigned int *pm4_fw;
	size_t pm4_fw_size;
	struct adreno_ringbuffer ringbuffer;
	unsigned int mharb;
	struct adreno_gpudev *gpudev;
	unsigned int wait_timeout;
	unsigned int istore_size;
	unsigned int pix_shader_start;
	unsigned int instruction_size;
};

struct adreno_gpudev {
	/*
	 * These registers are in a different location on A3XX,  so define
	 * them in the structure and use them as variables.
	 */
	unsigned int reg_rbbm_status;
	unsigned int reg_cp_pfp_ucode_data;
	unsigned int reg_cp_pfp_ucode_addr;

	/* GPU specific function hooks */
	int (*ctxt_create)(struct adreno_device *, struct adreno_context *);
	void (*ctxt_save)(struct adreno_device *, struct adreno_context *);
	void (*ctxt_restore)(struct adreno_device *, struct adreno_context *);
	irqreturn_t (*irq_handler)(struct adreno_device *);
	void (*irq_control)(struct adreno_device *, int);
	void (*rb_init)(struct adreno_device *, struct adreno_ringbuffer *);
	void (*start)(struct adreno_device *);
	unsigned int (*busy_cycles)(struct adreno_device *);
};

extern struct adreno_gpudev adreno_a2xx_gpudev;
extern struct adreno_gpudev adreno_a3xx_gpudev;

int adreno_idle(struct kgsl_device *device, unsigned int timeout);
void adreno_regread(struct kgsl_device *device, unsigned int offsetwords,
				unsigned int *value);
void adreno_regwrite(struct kgsl_device *device, unsigned int offsetwords,
				unsigned int value);

struct kgsl_memdesc *adreno_find_region(struct kgsl_device *device,
						unsigned int pt_base,
						unsigned int gpuaddr,
						unsigned int size);

uint8_t *adreno_convertaddr(struct kgsl_device *device,
	unsigned int pt_base, unsigned int gpuaddr, unsigned int size);

static inline int adreno_is_a200(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A200);
}

static inline int adreno_is_a203(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A203);
}

static inline int adreno_is_a205(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A205);
}

static inline int adreno_is_a20x(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev <= 209);
}

static inline int adreno_is_a220(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A220);
}

static inline int adreno_is_a225(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A225);
}

static inline int adreno_is_a22x(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev  == ADRENO_REV_A220 ||
		adreno_dev->gpurev == ADRENO_REV_A225);
}

static inline int adreno_is_a2xx(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev <= 299);
}

static inline int adreno_is_a3xx(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev >= 300);
}

/**
 * adreno_encode_istore_size - encode istore size in CP format
 * @adreno_dev - The 3D device.
 *
 * Encode the istore size into the format expected that the
 * CP_SET_SHADER_BASES and CP_ME_INIT commands:
 * bits 31:29 - istore size as encoded by this function
 * bits 27:16 - vertex shader start offset in instructions
 * bits 11:0 - pixel shader start offset in instructions.
 */
static inline int adreno_encode_istore_size(struct adreno_device *adreno_dev)
{
	unsigned int size;
	/* in a225 the CP microcode multiplies the encoded
	 * value by 3 while decoding.
	 */
	if (adreno_is_a225(adreno_dev))
		size = adreno_dev->istore_size/3;
	else
		size = adreno_dev->istore_size;

	return (ilog2(size) - 5) << 29;
}

#endif /*__ADRENO_H */
