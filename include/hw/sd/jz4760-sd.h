/*
 * JZ4760 SD controller Module
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the SD controller from the JZ4760 SoC.
 * The datasheet describes this as a single module with two sets
 * of registers, one per SD card. We implement it as a single
 * SD card controller and create two in the SoC, since the
 * two don't need to interact at all.a
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 * + sysbus IRQ: interrupt
 * + "sd-bus" bus: sd-bus for the SD card
 */

#ifndef HW_MISC_JZ4760_SD_H
#define HW_MISC_JZ4760_SD_H

#include "hw/sd/sd.h"
#include "qemu/fifo32.h"

#define TYPE_JZ4760_SD "jz4760-sd"
#define JZ4760_SD(obj) OBJECT_CHECK(JZ4760SD, obj, TYPE_JZ4760_SD)

#define TYPE_JZ4760_SD_BUS "jz4760-sd-bus"
#define JZ4760_SD_BUS(obj) OBJECT_CHECK(SDBus, obj, TYPE_JZ4760_SD_BUS)

#define JZ4760_SD_NUM_REGS 40

typedef struct JZ4760SD {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    SDBus sdbus;

    uint32_t stat;
    uint32_t clkrt;
    uint32_t cmdat;
    uint32_t resto;
    uint32_t rdto;
    uint32_t blklen;
    uint32_t nob;
    uint32_t snob;
    uint32_t imask;
    uint32_t ireg;
    uint32_t cmd;
    uint32_t arg;
    uint32_t lpm;

    /*
     * RES FIFO : includes one extra byte to allow for the
     * R2 response CRC byte, which the guest does not see
     * but which sdbus_do_command() writes.
     */
    uint8_t response[17];
    uint32_t response_read_ptr;
    uint32_t response_len;

    /*
     * RXFIFO/TXFIFO: a 16-entry 32-bit FIFO shared between
     * receive and transmit
     */
    Fifo32 datafifo;

    MemoryRegion iomem;
    qemu_irq irq;
} JZ4760SD;

#endif
