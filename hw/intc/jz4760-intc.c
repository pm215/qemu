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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "trace.h"
#include "hw/intc/jz4760-intc.h"

REG32(ICSR0, 0x0)
REG32(ICMR0, 0x4)
REG32(ICMSR0, 0x8)
REG32(ICMCR0, 0xc)
REG32(ICPR0, 0x10)
REG32(ICSR1, 0x20)
REG32(ICMR1, 0x24)
REG32(ICMSR1, 0x28)
REG32(ICMCR1, 0x2c)
REG32(ICPR1, 0x30)

static uint32_t icpr0(JZ4760INTC *s)
{
    return s->icsr0 & ~s->icmr0;
}

static uint32_t icpr1(JZ4760INTC *s)
{
    return s->icsr1 & ~s->icmr1;
}

static void jz4760_intc_update(JZ4760INTC *s)
{
    bool level = icpr0(s) || icpr1(s);

    qemu_set_irq(s->irq, level);
}

static uint64_t jz4760_intc_read(void *opaque, hwaddr addr, unsigned size)
{
    JZ4760INTC *s = opaque;
    uint64_t r = 0;

    switch (addr) {
    case A_ICSR0:
        r = s->icsr0;
        break;
    case A_ICMR0:
        r = s->icmr0;
        break;
    case A_ICPR0:
        r = icpr0(s);
        break;
    case A_ICSR1:
        r = s->icsr1;
        break;
    case A_ICMR1:
        r = s->icmr1;
        break;
    case A_ICPR1:
        r = icpr1(s);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 INTC read: bad offset 0x%x\n", (uint32_t)addr);
        r = 0;
        break;
    }

    trace_jz4760_intc_read(addr, r, size);
    return r;
}

static void jz4760_intc_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    JZ4760INTC *s = opaque;
    trace_jz4760_intc_write(addr, val, size);

    switch (addr) {
    case A_ICMR0:
        s->icmr0 = val;
        break;
    case A_ICMSR0:
        s->icmr0 |= val;
        break;
    case A_ICMCR0:
        s->icmr0 &= ~val;
        break;
    case A_ICMR1:
        s->icmr1 = val;
        break;
    case A_ICMSR1:
        s->icmr1 |= val;
        break;
    case A_ICMCR1:
        s->icmr1 &= ~val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 INTC write: bad offset 0x%x\n", (uint32_t)addr);
        break;
    }

    jz4760_intc_update(s);
}

static const MemoryRegionOps jz4760_intc_ops = {
    .read = jz4760_intc_read,
    .write = jz4760_intc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void jz4760_irq_set(void *opaque, int n, int level)
{
    JZ4760INTC *s = opaque;

    trace_jz4760_intc_irq_set(n, level);

    assert(n < 64);

    if (n < 32) {
        s->icsr0 |= 1 << n;
    } else {
        s->icsr1 |= 1 << (n - 32);
    }

    jz4760_intc_update(s);
}

static void jz4760_intc_reset(DeviceState *dev)
{
    JZ4760INTC *s = JZ4760_INTC(dev);

    s->icsr0 = 0;
    s->icsr1 = 0;
    s->icmr0 = 0xffffffff;
    s->icmr1 = 0xffffffff;
}

static void jz4760_intc_init(Object *obj)
{
    JZ4760INTC *s = JZ4760_INTC(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &jz4760_intc_ops, s,
                          "jz4760-intc", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void jz4760_intc_realize(DeviceState *dev, Error **errp)
{
    qdev_init_gpio_in(dev, jz4760_irq_set, 64);
}

static const VMStateDescription jz4760_intc_vmstate = {
    .name = "jz4760-intc",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(icsr0, JZ4760INTC),
        VMSTATE_UINT32(icsr1, JZ4760INTC),
        VMSTATE_UINT32(icmr0, JZ4760INTC),
        VMSTATE_UINT32(icmr1, JZ4760INTC),
        VMSTATE_END_OF_LIST()
    }
};

static void jz4760_intc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_intc_realize;
    dc->vmsd = &jz4760_intc_vmstate;
    dc->reset = jz4760_intc_reset;
}

static const TypeInfo jz4760_intc_info = {
    .name = TYPE_JZ4760_INTC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760INTC),
    .instance_init = jz4760_intc_init,
    .class_init = jz4760_intc_class_init,
};

static void jz4760_intc_register_types(void)
{
    type_register_static(&jz4760_intc_info);
}

type_init(jz4760_intc_register_types);
