/*
 * JZ4760 Timer Counter Unit
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the Timer Counter Unit from the JZ4760 SoC
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 * + sysbus IRQ 0: interrupt for OST
 * + sysbus IRQ 1: interrupt for timer 0
 * + sysbus IRQ 2: interrupt for timers 1-7
 */

#ifndef HW_MISC_JZ4760_TCU_H
#define HW_MISC_JZ4760_TCU_H

#define TYPE_JZ4760_TCU "jz4760-tcu"
#define JZ4760_TCU(obj) OBJECT_CHECK(JZ4760TCU, obj, TYPE_JZ4760_TCU)

#define JZ4760_TCU_NUM_REGS 40

typedef struct JZ4760TCUCounter {
    uint16_t tdfr;
    uint16_t tdhr;
    uint16_t tcnt;
    uint16_t tcsr;
} JZ4760TCUCounter;

#define JZ4760TCU_NUM_COUNTERS 8

typedef struct JZ4760TCU {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    JZ4760TCUCounter counter[JZ4760TCU_NUM_COUNTERS];

    uint32_t tsr;
    uint16_t ter;
    uint32_t tfr;
    uint32_t tmr;

    uint32_t ostdr;
    uint32_t ostcnt;
    uint16_t ostcsr;

    uint16_t wdtdr;
    uint8_t wdtcer;
    uint16_t wdtcnt;
    uint16_t wdtcsr;

    MemoryRegion iomem;
    qemu_irq irq[3];
} JZ4760TCU;

#endif
