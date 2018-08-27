/*
 * JZ4760 Interrupt Controller Module
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the Interrupt Controller Module from the JZ4760 SoC
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 * + GPIO inputs: 64 interrupt lines
 * + sysbus IRQ 0: outbound IRQ to CPU
 */

#ifndef HW_MISC_JZ4760_INTC_H
#define HW_MISC_JZ4760_INTC_H

#define TYPE_JZ4760_INTC "jz4760-intc"
#define JZ4760_INTC(obj) OBJECT_CHECK(JZ4760INTC, obj, TYPE_JZ4760_INTC)

typedef struct JZ4760INTC {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    uint32_t icsr0;
    uint32_t icsr1;
    uint32_t icmr0;
    uint32_t icmr1;

    MemoryRegion iomem;

    qemu_irq irq;
} JZ4760INTC;

#endif
