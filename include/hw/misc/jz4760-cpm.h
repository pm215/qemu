/*
 * JZ4760 Clock and Power Module
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the Clock and Power Module from the JZ4760 SoC
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 */

#ifndef HW_MISC_JZ4760_CPM_H
#define HW_MISC_JZ4760_CPM_H

#define TYPE_JZ4760_CPM "jz4760-cpm"
#define JZ4760_CPM(obj) OBJECT_CHECK(JZ4760CPM, obj, TYPE_JZ4760_CPM)

#define JZ4760_CPM_NUM_REGS 40

typedef struct JZ4760CPM {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    uint32_t regs[JZ4760_CPM_NUM_REGS];

    MemoryRegion iomem;
} JZ4760CPM;

#endif
