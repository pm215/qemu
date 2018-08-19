/*
 * JZ4760 MIPS SoC
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the JZ4760 MIPS SoC
 * https://www.rockbox.org/wiki/pub/Main/IngenicJz47xx/JZ4760_pm.pdf
 *
 * QEMU interface:
 *  + QOM property "memory" is a MemoryRegion containing the devices provided
 *    by the board model.
 */

#ifndef MIPS_JZ4760_H
#define MIPS_JZ4760_H

#include "hw/sysbus.h"
#include "hw/misc/unimp.h"
#include "hw/intc/jz4760-intc.h"
#include "hw/misc/jz4760-cpm.h"
#include "hw/dma/jz4760-dma.h"
#include "target/mips/cpu.h"

#define TYPE_JZ4760 "jz4760"
#define JZ4760(obj) OBJECT_CHECK(JZ4760, (obj), TYPE_JZ4760)

typedef struct JZ4760 {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion container;
    MIPSCPU *cpu;
    MemoryRegion bootrom;
    MemoryRegion sram_alias;
    JZ4760INTC intc;
    JZ4760CPM cpm;
    JZ4760DMA mdmac;
    JZ4760DMA dmac;
    JZ4760DMA bdmac;

    /* Properties */
    MemoryRegion *board_memory;
} JZ4760;

#endif
