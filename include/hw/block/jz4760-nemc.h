/*
 * JZ4760 NAND and External Memory Controller
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the NEMC Module from the JZ4760 SoC
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 * + sysbus MMIO region 1: direct-mapped NAND-control region
 * + QOM property "nand": pointer to the DeviceState which is the TYPE_NAND
 *   device for CS1. The real hardware allows up to 6 NAND devices; we only
 *   model connecting one for the moment.
 */

#ifndef HW_MISC_JZ4760_NEMC_H
#define HW_MISC_JZ4760_NEMC_H

#define TYPE_JZ4760_NEMC "jz4760-nemc"
#define JZ4760_NEMC(obj) OBJECT_CHECK(JZ4760NEMC, obj, TYPE_JZ4760_NEMC)

#define JZ4760_NEMC_NUM_CS 6

typedef struct JZ4760NEMC {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/

    uint32_t smcr[JZ4760_NEMC_NUM_CS];
    uint32_t sacr[JZ4760_NEMC_NUM_CS];
    uint32_t nfcsr;
    uint32_t pncr;
    uint32_t pndr;
    uint32_t bitcnt;

    MemoryRegion iomem_regs;
    MemoryRegion iomem_nand;

    DeviceState *nanddev;
} JZ4760NEMC;

#endif
