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

#include "qemu/timer.h"

#define TYPE_JZ4760_TCU "jz4760-tcu"
#define JZ4760_TCU(obj) OBJECT_CHECK(JZ4760TCU, obj, TYPE_JZ4760_TCU)

#define JZ4760_TCU_NUM_REGS 40

/* Encapsulate some of the common logic of an upcounter */
typedef struct JZ4760UpCounter {
    QEMUTimer timer;
    bool enabled;
    /* Count value if counter is stopped; otherwise value as of last_event */
    uint32_t count;
    /* Comparison value */
    uint32_t compare;
    /* QEMU_CLOCK_VIRTUAL ns time when we last synced count */
    int64_t last_event;
    /* Counter clock frequency in Hz */
    uint32_t frq;
} JZ4760UpCounter;

typedef struct JZ4760TCU JZ4760TCU;

typedef struct JZ4760TCUCounter {
    JZ4760UpCounter upcounter;
    JZ4760TCU *parent;
    uint16_t tdfr;
    uint16_t tdhr;
    uint16_t tcnt;
    uint16_t tcsr;
} JZ4760TCUCounter;

#define JZ4760TCU_NUM_COUNTERS 8

struct JZ4760TCU {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    JZ4760TCUCounter counter[JZ4760TCU_NUM_COUNTERS];

    uint32_t tsr;
    uint16_t ter;
    uint32_t tfr;
    uint32_t tmr;

    uint32_t ostdr;
    uint16_t ostcsr;

    uint16_t wdtdr;
    uint8_t wdtcer;
    uint16_t wdtcnt;
    uint16_t wdtcsr;

    JZ4760UpCounter oscounter;
    JZ4760UpCounter wdcounter;

    MemoryRegion iomem;
    qemu_irq irq[3];
};

#endif
