/*
 * JZ4760 DMA Module
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the DMA Module from the JZ4760 SoC.
 * The SoC has three DMA modules (MDMAC, DMAC, BDMAC) which
 * are differently configured flavours of the same hardware.
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 * + QOM property "num-cores": number of DMA cores
 * + QOM property "num-channels": number of channels per core
 * + QOM property "downstrem": MemoryRegion defining where DMA
 *   bus master transactions are made
 */

#ifndef HW_DMA_JZ4760_DMA_H
#define HW_DMA_JZ4760_DMA_H

#define TYPE_JZ4760_DMA "jz4760-dma"
#define JZ4760_DMA(obj) OBJECT_CHECK(JZ4760DMA, obj, TYPE_JZ4760_DMA)

#define JZ4760_DMA_MAX_CHANNELS 5
#define JZ4760_DMA_MAX_CORES 2

typedef struct JZ4760DMAChannel {
    uint32_t dsa;
    uint32_t dta;
    uint32_t dtc;
    uint32_t drt;
    uint32_t dcs;
    uint32_t dcm;
    uint32_t dda;
    uint32_t dsd;
} JZ4760DMAChannel;

typedef struct JZ4760DMACore {
    JZ4760DMAChannel channel[JZ4760_DMA_MAX_CHANNELS];
    uint32_t dmac;
    uint32_t dirqp;
    uint32_t ddr;
    uint32_t dcke;
} JZ4760DMACore;

typedef struct JZ4760DMA {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;
    MemoryRegion *downstream;
    AddressSpace downstream_as;
    uint32_t num_channels;
    uint32_t num_cores;

    JZ4760DMACore core[JZ4760_DMA_MAX_CORES];
} JZ4760DMA;

#endif
