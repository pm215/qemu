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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "trace.h"
#include "hw/gpio/jz4760-gpio.h"

REG32(PAPIN, 0x0)

static uint64_t jz4760_gpio_read(void *opaque, hwaddr addr, unsigned size)
{
    //JZ4760GPIO *s = opaque;
    uint64_t r = 0;

    switch (addr) {
    case A_PAPIN:
        /* NAND ready line asserted, nothing else */
        r = 0x00100000;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 GPIO read: bad offset 0x%x\n", (uint32_t)addr);
        r = 0;
        break;
    }

    trace_jz4760_gpio_read(addr, r, size);
    return r;
}

static void jz4760_gpio_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    //JZ4760GPIO *s = opaque;
    trace_jz4760_gpio_write(addr, val, size);

    switch (addr) {
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 GPIO write: bad offset 0x%x\n", (uint32_t)addr);
        break;
    }
}

static const MemoryRegionOps jz4760_gpio_ops = {
    .read = jz4760_gpio_read,
    .write = jz4760_gpio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void jz4760_gpio_reset(DeviceState *dev)
{
//    JZ4760GPIO *s = JZ4760_GPIO(dev);
}

static void jz4760_gpio_init(Object *obj)
{
    JZ4760GPIO *s = JZ4760_GPIO(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &jz4760_gpio_ops, s,
                          "jz4760-gpio", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void jz4760_gpio_realize(DeviceState *dev, Error **errp)
{
}

static const VMStateDescription jz4760_gpio_vmstate = {
    .name = "jz4760-gpio",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void jz4760_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_gpio_realize;
    dc->vmsd = &jz4760_gpio_vmstate;
    dc->reset = jz4760_gpio_reset;
}

static const TypeInfo jz4760_gpio_info = {
    .name = TYPE_JZ4760_GPIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760GPIO),
    .instance_init = jz4760_gpio_init,
    .class_init = jz4760_gpio_class_init,
};

static void jz4760_gpio_register_types(void)
{
    type_register_static(&jz4760_gpio_info);
}

type_init(jz4760_gpio_register_types);
