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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "trace.h"
#include "hw/dma/jz4760-dma.h"

/*
 * Each channel has 8 registers, and successive channels are 0x20 apart.
 * The first DMA core's bank of channel registers is at offset 0x0;
 * if there is a second core, its bank starts at offset 0x100.
 */
REG32(DSA, 0x0)
REG32(DTA, 0x4)
REG32(DTC, 0x8)
REG32(DRT, 0xc)
REG32(DCS, 0x10)
REG32(DCM, 0x14)
REG32(DDA, 0x18)
REG32(DSD, 0x1c)
/*
 * As well as its bank of channel registers, each core has five registers
 * which control it; the first core's control registers start at 0x300,
 * and those for the second core (if present) at 0x400.
 */
REG32(DMAC, 0x0)
REG32(DIRQP, 0x4)
REG32(DDR, 0x8)
REG32(DDRS, 0xc)
REG32(DCKE, 0x10)

static void jz4760_dma_run_channel(JZ4760DMA *s, int core, int channel)
{
    /* Try to actually do some DMA for this core and channel */
}

static void jz4760_dma_run_core(JZ4760DMA *s, int core)
{
    /* Try to actually do some DMA for this core, any channel */
    int i;

    for (i = 0; i < s->num_channels; i++) {
        jz4760_dma_run_channel(s, core, i);
    }
}

static uint64_t jz4760_dma_read(void *opaque, hwaddr addr, unsigned size)
{
    JZ4760DMA *s = opaque;
    JZ4760DMAChannel *ch;
    JZ4760DMACore *c;
    uint64_t r;

    if (addr < 0x300) {
        int core = extract32(addr, 8, 2);
        int channel = extract32(addr, 5, 3);
        if (channel >= s->num_channels || core >= s->num_cores) {
            goto bad_offset;
        }

        c = &s->core[core];
        ch = &c->channel[channel];

        switch (extract32(addr, 0, 5)) {
        case A_DSA:
            r = ch->dsa;
            break;
        case A_DTA:
            r = ch->dta;
            break;
        case A_DTC:
            r = ch->dtc;
            break;
        case A_DRT:
            r = ch->drt;
            break;
        case A_DCS:
            r = ch->dcs;
            break;
        case A_DCM:
            r = ch->dcm;
            break;
        case A_DDA:
            r = ch->dda;
            break;
        case A_DSD:
            r = ch->dsd;
            break;
        default:
            goto bad_offset;
        }
    } else {
        int core = extract32(addr - 0x300, 8, 2);
        if (core >= s->num_cores) {
            goto bad_offset;
        }

        c = &s->core[core];

        switch (extract32(addr, 0, 8)) {
        case A_DMAC:
            r = c->dmac;
            break;
        case A_DIRQP:
            r = c->dirqp;
            break;
        case A_DDR:
            r = c->ddr;
            break;
        case A_DCKE:
            r = c->dcke;
            break;
        default:
        bad_offset:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "jz4760 DMA read: bad offset 0x%x\n", (uint32_t)addr);
            r = 0;
            break;
        }
    }

    trace_jz4760_dma_read(addr, r, size);
    return r;
}

static void jz4760_dma_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    JZ4760DMA *s = opaque;
    JZ4760DMAChannel *ch;
    JZ4760DMACore *c;

    trace_jz4760_dma_write(addr, val, size);

    if (addr < 0x300) {
        int core = extract32(addr, 8, 2);
        int channel = extract32(addr, 5, 3);
        if (channel >= s->num_channels || core >= s->num_cores) {
            goto bad_offset;
        }

        c = &s->core[core];
        ch = &c->channel[channel];

        switch (extract32(addr, 0, 5)) {
        case A_DSA:
            ch->dsa = val;
            break;
        case A_DTA:
            ch->dta = val;
            break;
        case A_DTC:
            ch->dtc = val;
            break;
        case A_DRT:
            ch->drt = val;
            break;
        case A_DCS:
            ch->dcs = val;
            jz4760_dma_run_channel(s, core, channel);
            break;
        case A_DCM:
            ch->dcm = val;
            break;
        case A_DDA:
            ch->dda = val;
            break;
        case A_DSD:
            ch->dsd = val;
            break;
        default:
            goto bad_offset;
        }
    } else {
        int core = extract32(addr - 0x300, 8, 2);
        if (core >= s->num_cores) {
            goto bad_offset;
        }

        c = &s->core[core];

        switch (extract32(addr, 0, 8)) {
        case A_DMAC:
            c->dmac = val;
            jz4760_dma_run_core(s, core);
            break;
        case A_DIRQP:
            c->dirqp = val;
            break;
        case A_DDRS:
            /* Writing 1 sets a doorbell bit; writing 0 is ignored */
            c->ddr |= val;
            jz4760_dma_run_core(s, core);
            break;
        case A_DCKE:
            c->dcke = val;
            break;
        default:
        bad_offset:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "jz4760 DMA write: bad offset 0x%x\n",
                          (uint32_t)addr);
            break;
        }
    }
}

static const MemoryRegionOps jz4760_dma_ops = {
    .read = jz4760_dma_read,
    .write = jz4760_dma_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void jz4760_dma_reset(DeviceState *dev)
{
    JZ4760DMA *s = JZ4760_DMA(dev);
    int channel, core;

    for (core = 0; core < s->num_cores; core++) {
        JZ4760DMACore *c = &s->core[core];

        c->dmac = 0;
        c->dirqp = 0;
        c->ddr = 0;
        c->dcke = 0;

        for (channel = 0; channel < s->num_channels; channel++) {
            JZ4760DMAChannel *ch = &c->channel[channel];

            ch->dsa = 0;
            ch->dta = 0;
            ch->dtc = 0;
            ch->drt = 0;
            ch->dcs = 0;
            ch->dcm = 0;
            ch->dda = 0;
            ch->dsd = 0;
        }
    }
}

static void jz4760_dma_init(Object *obj)
{
    JZ4760DMA *s = JZ4760_DMA(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &jz4760_dma_ops, s,
                          "jz4760-dma", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void jz4760_dma_realize(DeviceState *dev, Error **errp)
{
    JZ4760DMA *s = JZ4760_DMA(dev);

    if (!s->downstream) {
        error_setg(errp, "jz4760-dma: 'downstream' link not set");
        return;
    }
    if (s->num_channels == 0 || s->num_channels > JZ4760_DMA_MAX_CHANNELS) {
        error_setg(errp, "jz4760-dma: 'num-channels' %d not valid", s->num_channels);
        return;
    }
    if (s->num_cores == 0 || s->num_cores > JZ4760_DMA_MAX_CORES) {
        error_setg(errp, "jz4760-dma: 'num-cores' %d not valid", s->num_cores);
        return;
    }

    address_space_init(&s->downstream_as, s->downstream,
                       "jz4760-dma-downstream");
}

static const VMStateDescription jz4760_channel_vmstate = {
    .name = "jz4760-dma-channel",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(dsa, JZ4760DMAChannel),
        VMSTATE_UINT32(dta, JZ4760DMAChannel),
        VMSTATE_UINT32(dtc, JZ4760DMAChannel),
        VMSTATE_UINT32(drt, JZ4760DMAChannel),
        VMSTATE_UINT32(dcs, JZ4760DMAChannel),
        VMSTATE_UINT32(dcm, JZ4760DMAChannel),
        VMSTATE_UINT32(dda, JZ4760DMAChannel),
        VMSTATE_UINT32(dsd, JZ4760DMAChannel),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription jz4760_core_vmstate = {
    .name = "jz4760-dma-core",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT_ARRAY(channel, JZ4760DMACore,
                             JZ4760_DMA_MAX_CHANNELS,
                             1, jz4760_channel_vmstate,
                             JZ4760DMAChannel),
        VMSTATE_UINT32(dmac, JZ4760DMACore),
        VMSTATE_UINT32(dirqp, JZ4760DMACore),
        VMSTATE_UINT32(ddr, JZ4760DMACore),
        VMSTATE_UINT32(dcke, JZ4760DMACore),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription jz4760_dma_vmstate = {
    .name = "jz4760-dma",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT_ARRAY(core, JZ4760DMA,
                             JZ4760_DMA_MAX_CORES,
                             1, jz4760_core_vmstate,
                             JZ4760DMACore),
        VMSTATE_END_OF_LIST()
    }
};

static Property jz4760_dma_properties[] = {
    DEFINE_PROP_LINK("downstream", JZ4760DMA, downstream,
                     TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_UINT32("num-channels", JZ4760DMA, num_channels, 1),
    DEFINE_PROP_UINT32("num-cores", JZ4760DMA, num_cores, 1),
    DEFINE_PROP_END_OF_LIST(),
};

static void jz4760_dma_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_dma_realize;
    dc->vmsd = &jz4760_dma_vmstate;
    dc->reset = jz4760_dma_reset;
    dc->props = jz4760_dma_properties;
}

static const TypeInfo jz4760_dma_info = {
    .name = TYPE_JZ4760_DMA,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760DMA),
    .instance_init = jz4760_dma_init,
    .class_init = jz4760_dma_class_init,
};

static void jz4760_dma_register_types(void)
{
    type_register_static(&jz4760_dma_info);
}

type_init(jz4760_dma_register_types);
