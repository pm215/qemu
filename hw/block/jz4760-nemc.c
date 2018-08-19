/*
 * JZ4760 NAND and External Memory Controller Module
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
#include "hw/block/jz4760-nemc.h"
#include "hw/block/flash.h"

/*
 * Listed in the three groups the manual puts them in; in address order
 * they are mixed up together. Some offsets are unused.
 */
REG32(SMCR1, 0x14)
REG32(SMCR2, 0x18)
REG32(SMCR3, 0x1c)
REG32(SMCR4, 0x20)
REG32(SMCR5, 0x24)
REG32(SMCR6, 0x28)
REG32(SACR1, 0x34)
REG32(SACR2, 0x38)
REG32(SACR3, 0x3c)
REG32(SACR4, 0x40)
REG32(SACR5, 0x44)
REG32(SACR6, 0x48)
    FIELD(SACR, MASK, 0, 8)
    FIELD(SACR, BASE, 8, 8)
#define R_SACR_VALID_MASK (R_SACR_MASK_MASK | R_SACR_BASE_MASK)
/*
 * QEMU will never care about any of the memory cycle times in SMCR, so
 * we just read-as-written. Define the valid bits, but don't bother
 * defining macros for every field in the register.
 */
#define R_SMCR_VALID_MASK 0x1ff33c3

REG32(NFCSR, 0x50)
REG32(PNCR, 0x100)
    FIELD(PNCR, PNEN, 0, 1)
    FIELD(PNCR, PNRST, 1, 1)
    FIELD(PNCR, BIT_EN, 3, 1)
    FIELD(PNCR, BIT_SEL, 4, 1)
    FIELD(PNCR, BIT_RST, 5, 1)
REG32(PNDR, 0x104)
    FIELD(PNDR, PNDR, 0, 23)
REG32(BITCNT, 0x108)

static uint64_t jz4760_nemc_read(void *opaque, hwaddr addr, unsigned size)
{
    JZ4760NEMC *s = opaque;
    uint64_t r = 0;

    switch (addr) {
    case A_SMCR1 ... A_SMCR6:
        r = s->smcr[(addr - A_SMCR1) / 4];
        break;
    case A_SACR1 ... A_SACR6:
        r = s->sacr[(addr - A_SACR1) / 4];
        break;
    case A_NFCSR:
        r = s->nfcsr;
        break;
    case A_PNCR:
        r = s->pncr;
        break;
    case A_PNDR:
        r = s->pndr;
        break;
    case A_BITCNT:
        r = s->bitcnt;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 NEMC read: bad offset 0x%x\n", (uint32_t)addr);
        r = 0;
        break;
    }

    trace_jz4760_nemc_read(addr, r, size);
    return r;
}

static void jz4760_nemc_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    JZ4760NEMC *s = opaque;
    trace_jz4760_nemc_write(addr, val, size);

    switch (addr) {
    case A_SMCR1 ... A_SMCR6:
        s->smcr[(addr - A_SMCR1) / 4] = val & R_SMCR_VALID_MASK;
        break;
    case A_SACR1 ... A_SACR6:
        s->sacr[(addr - A_SACR1) / 4] = val & R_SACR_VALID_MASK;
        break;
    case A_NFCSR:
        s->nfcsr = val & MAKE_64BIT_MASK(0, JZ4760_NEMC_NUM_CS * 2);
        break;
    case A_PNCR:
        /* Other bits are reserved or write-only */
        s->pncr = val & (R_PNCR_PNEN_MASK |
                         R_PNCR_BIT_EN_MASK |
                         R_PNCR_BIT_SEL_MASK);
        if (val & R_PNCR_BIT_RST_MASK) {
            s->bitcnt = 0;
        }
        if (val & R_PNCR_PNRST_MASK) {
            s->pndr = 0xa5a5;
        }
        break;
    case A_PNDR:
        s->pndr = val & R_PNDR_PNDR_MASK;
        break;
    case A_BITCNT:
        s->bitcnt = val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 NEMC write: bad offset 0x%x\n", (uint32_t)addr);
        break;
    }
}

/*
 * NAND command, address and data cycles are performed by accesses to addresses
 * in the range 0x14000000..0x1bffffff, which effectively encode which NAND
 * device to use and whether to perform a command, address or data cycle.
 * Decode the ALE, CLE and chipselect from the offset. Return false for
 * a bad address, otherwise true and ale, cle, cs are filled in.
 */
static bool nand_decode(hwaddr offset, bool *ale, bool *cle, int *cs)
{
    /*
     * The chipselect almost but doesn't quite decode neatly from the upper
     * address bits. We count our chip selects from 0, unlike the data sheet.
     */
    int csbits = extract32(offset, 24, 3);

    switch (csbits) {
    case 0 ... 4: /* absolute addresses 0x14xxxxxx ... 0x18xxxxxx */
        *cs = 5 - csbits;
        break;
    case 6: /* absolute addresses 0x1axxxxxx */
        *cs = 0;
        break;
    case 5: /* absolute addresses 0x19xxxxxx */
    case 7: /* absolute addresses 0x1bxxxxxx */
        return false;
    }
    *cle = extract32(offset, 22, 1);
    *ale = extract32(offset, 23, 1);
    return true;
}

static uint64_t jz4760_nemc_nand_read(void *opaque, hwaddr addr, unsigned size)
{
    JZ4760NEMC *s = opaque;
    uint64_t r;
    bool ale, cle;
    int cs;

    if (!nand_decode(addr, &ale, &cle, &cs)) {
        goto bad_offset;
    }

    if (ale == 1 || cle == 1) {
        /* For reads, only data cycles are valid */
        goto bad_offset;
    }
    if (cs != 0 || !s->nanddev) {
        /*
         * TODO is this the right way to handle "no NAND actually
         * connected to this chip select" ?
         */
        goto bad_offset;
    }

    nand_setpins(s->nanddev, cle, ale, 0, 1, 0);
    r = nand_getio(s->nanddev);

    trace_jz4760_nemc_nand_read(addr, r, size);
    return r;

bad_offset:
    qemu_log_mask(LOG_GUEST_ERROR,
                  "jz4760 NEMC NAND read: bad offset 0x%x\n", (uint32_t)addr);
    return 0;
}

static void jz4760_nemc_nand_write(void *opaque, hwaddr addr, uint64_t val,
                                   unsigned size)
{
    JZ4760NEMC *s = opaque;
    bool ale, cle;
    int cs;

    trace_jz4760_nemc_nand_write(addr, val, size);

    if (!nand_decode(addr, &ale, &cle, &cs)) {
        goto bad_offset;
    }

    if (ale == 1 && cle == 1) {
        /* For writes, trying both ALE and CLE at once is invalid */
        goto bad_offset;
    }
    if (cs != 0 || !s->nanddev) {
        /*
         * TODO is this the right way to handle "no NAND actually
         * connected to this chip select" ?
         */
        goto bad_offset;
    }

    nand_setpins(s->nanddev, cle, ale, 0, 1, 0);
    nand_setio(s->nanddev, val);
    return;

bad_offset:
    qemu_log_mask(LOG_GUEST_ERROR,
                  "jz4760 NEMC NAND write: bad offset 0x%x\n", (uint32_t)addr);
}

static const MemoryRegionOps jz4760_nemc_ops = {
    .read = jz4760_nemc_read,
    .write = jz4760_nemc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static const MemoryRegionOps jz4760_nemc_nand_ops = {
    .read = jz4760_nemc_nand_read,
    .write = jz4760_nemc_nand_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 2,
    .valid.min_access_size = 1,
    .valid.max_access_size = 2,
};

static void jz4760_nemc_reset(DeviceState *dev)
{
    JZ4760NEMC *s = JZ4760_NEMC(dev);
    int i;

    for (i = 0; i < JZ4760_NEMC_NUM_CS; i++) {
        s->smcr[i] = 0x0fff7700;
    }
    /* Note that SACR1 is sacr[0], and so on */
    s->sacr[0] = 0x1afe;
    s->sacr[1] = 0x18fe;
    s->sacr[2] = 0x17ff;
    s->sacr[3] = 0x16ff;
    s->sacr[4] = 0x15ff;
    s->sacr[5] = 0x14ff;

    s->nfcsr = 0;
    s->pncr = 0;
    s->pndr = 0x5aa5;
    s->bitcnt = 0;
}

static void jz4760_nemc_init(Object *obj)
{
    JZ4760NEMC *s = JZ4760_NEMC(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem_regs, obj, &jz4760_nemc_ops, s,
                          "jz4760-nemc", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem_regs);
    memory_region_init_io(&s->iomem_nand, obj, &jz4760_nemc_nand_ops, s,
                          "jz4760-nemc-nand", 0x08000000);
    sysbus_init_mmio(sbd, &s->iomem_nand);
}

static void jz4760_nemc_realize(DeviceState *dev, Error **errp)
{
}

static const VMStateDescription jz4760_nemc_vmstate = {
    .name = "jz4760-nemc",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(smcr, JZ4760NEMC, JZ4760_NEMC_NUM_CS),
        VMSTATE_UINT32_ARRAY(sacr, JZ4760NEMC, JZ4760_NEMC_NUM_CS),
        VMSTATE_UINT32(nfcsr, JZ4760NEMC),
        VMSTATE_UINT32(pncr, JZ4760NEMC),
        VMSTATE_UINT32(pndr, JZ4760NEMC),
        VMSTATE_UINT32(bitcnt, JZ4760NEMC),
        VMSTATE_END_OF_LIST()
    }
};

static Property jz4760_nemc_properties[] = {
    DEFINE_PROP_LINK("nand", JZ4760NEMC, nanddev, TYPE_DEVICE,
                     DeviceState *),
    DEFINE_PROP_END_OF_LIST()
};

static void jz4760_nemc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_nemc_realize;
    dc->vmsd = &jz4760_nemc_vmstate;
    dc->reset = jz4760_nemc_reset;
    dc->props = jz4760_nemc_properties;
}

static const TypeInfo jz4760_nemc_info = {
    .name = TYPE_JZ4760_NEMC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760NEMC),
    .instance_init = jz4760_nemc_init,
    .class_init = jz4760_nemc_class_init,
};

static void jz4760_nemc_register_types(void)
{
    type_register_static(&jz4760_nemc_info);
}

type_init(jz4760_nemc_register_types);
