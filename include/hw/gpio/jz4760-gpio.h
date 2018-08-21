/*
 * JZ4760 GPIO Module
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your option) any later version.
 */

/* This is a model of the GPIO Module from the JZ4760 SoC
 *
 * QEMU interface:
 * + sysbus MMIO region 0: registers
 */

#ifndef HW_GPIO_JZ4760_GPIO_H
#define HW_GPIO_JZ4760_GPIO_H

#define TYPE_JZ4760_GPIO "jz4760-gpio"
#define JZ4760_GPIO(obj) OBJECT_CHECK(JZ4760GPIO, obj, TYPE_JZ4760_GPIO)

#define JZ4760_GPIO_NUM_REGS 40

typedef struct JZ4760GPIO {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/

    MemoryRegion iomem;
} JZ4760GPIO;

#endif
