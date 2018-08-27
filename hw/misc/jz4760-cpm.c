/*
 * JZ4760 Clock and Power Module
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
#include "hw/misc/jz4760-cpm.h"

/*
 * Listed in the three groups the manual puts them in; in address order
 * they are mixed up together. Some offsets are unused.
 */
REG32(CPCCR, 0x0)
REG32(CPPCR, 0x10)
    FIELD(CPPCR, PLLST, 0, 8)
    FIELD(CPPCR, PLLEN, 8, 1)
    FIELD(CPPCR, PLLBP, 9, 1)
    FIELD(CPPCR, PLLS, 10, 1)
    FIELD(CPPCR, ENLOCK, 14, 1)
    FIELD(CPPCR, LOCK0, 15, 1)
    FIELD(CPPCR, PLLOD, 16, 2)
    FIELD(CPPCR, PLLN, 18, 4)
    FIELD(CPPCR, PLLM, 24, 7)
#define R_CPPCR_VALID_MASK (R_CPPCR_PLLST_MASK | R_CPPCR_PLLEN_MASK |   \
                            R_CPPCR_PLLBP_MASK | R_CPPCR_PLLS_MASK |    \
                            R_CPPCR_ENLOCK_MASK | R_CPPCR_LOCK0_MASK |  \
                            R_CPPCR_PLLOD_MASK | R_CPPCR_PLLN_MASK |    \
                            R_CPPCR_PLLM_MASK)
REG32(CPPSR, 0x14)
REG32(CPPCR1, 0x30)
    FIELD(CPPCR1, PLLON, 0, 1)
    FIELD(CPPCR1, PLLOFF, 1, 1)
    FIELD(CPPCR1, LOCK1, 2, 1)
    FIELD(CPPCR1, PLL1S, 6, 1)
    FIELD(CPPCR1, PLL1EN, 7, 1)
    FIELD(CPPCR1, P1SDIV, 9, 6)
    FIELD(CPPCR1, P1SCS, 15, 1)
    FIELD(CPPCR1, PLL1OD, 16, 2)
    FIELD(CPPCR1, PLL1N, 18, 4)
    FIELD(CPPCR1, PLL1M, 24, 7)
#define R_CPPCR1_VALID_MASK (R_CPPCR1_PLLON_MASK | R_CPPCR1_PLLOFF_MASK | \
                             R_CPPCR1_LOCK1_MASK | R_CPPCR1_PLL1S_MASK | \
                             R_CPPCR1_PLL1EN_MASK | R_CPPCR1_P1SDIV_MASK | \
                             R_CPPCR1_P1SCS_MASK | R_CPPCR1_PLL1OD_MASK | \
                             R_CPPCR1_PLL1N_MASK | R_CPPCR1_PLL1M_MASK)
REG32(CPSPR, 0x34)
REG32(CPSPPR, 0x38)
REG32(USBPCR, 0x3c)
REG32(USBRDT, 0x40)
REG32(USBVBFIL, 0x44)
REG32(USBCDR, 0x50)
REG32(I2SCDR, 0x60)
REG32(LPCDR, 0x64)
REG32(MSCCDR, 0x68)
REG32(UHCCDR, 0x6c)
REG32(SSICDR, 0x74)
REG32(CIMCDR, 0x7c)
REG32(GPSCDR, 0x80)
REG32(PCMCDR, 0x84)
REG32(GPUCDR, 0x88)

/* Power management registers */
REG32(LCR, 0x4)
REG32(PSWC0ST, 0x90)
REG32(PSWC1ST, 0x94)
REG32(PSWC2ST, 0x98)
REG32(PSWC3ST, 0x9c)
REG32(CLKGR0, 0x20)
REG32(OPCR, 0x24)
REG32(CLKGR1, 0x28)

/* Reset control registers */
REG32(RSR, 0x8)

static uint64_t jz4760_cpm_read(void *opaque, hwaddr addr, unsigned size)
{
    JZ4760CPM *s = opaque;
    uint64_t r = 0;

    switch (addr) {
    case A_CPCCR:
    case A_CPPCR:
    case A_CPPSR:
    case A_CPPCR1:
    case A_CPSPR:
    case A_CPSPPR:
    case A_USBPCR:
    case A_USBRDT:
    case A_USBVBFIL:
    case A_USBCDR:
    case A_I2SCDR:
    case A_LPCDR:
    case A_MSCCDR:
    case A_UHCCDR:
    case A_SSICDR:
    case A_CIMCDR:
    case A_GPSCDR:
    case A_PCMCDR:
    case A_GPUCDR:
    case A_LCR:
    case A_PSWC0ST:
    case A_PSWC1ST:
    case A_PSWC2ST:
    case A_PSWC3ST:
    case A_CLKGR0:
    case A_OPCR:
    case A_CLKGR1:
    case A_RSR:
        r = s->regs[addr / 4];
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 CPM read: bad offset 0x%x\n", (uint32_t)addr);
        r = 0;
        break;
    }

    trace_jz4760_cpm_read(addr, r, size);
    return r;
}

static void jz4760_cpm_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    JZ4760CPM *s = opaque;
    trace_jz4760_cpm_write(addr, val, size);

    switch (addr) {
    case A_CPCCR:
    case A_CPPSR:
    case A_CPSPR:
    case A_CPSPPR:
    case A_USBPCR:
    case A_USBRDT:
    case A_USBVBFIL:
    case A_USBCDR:
    case A_I2SCDR:
    case A_LPCDR:
    case A_MSCCDR:
    case A_UHCCDR:
    case A_SSICDR:
    case A_CIMCDR:
    case A_GPSCDR:
    case A_PCMCDR:
    case A_GPUCDR:
    case A_LCR:
    case A_PSWC0ST:
    case A_PSWC1ST:
    case A_PSWC2ST:
    case A_PSWC3ST:
    case A_CLKGR0:
    case A_OPCR:
    case A_CLKGR1:
        s->regs[addr / 4] = val;
        break;
    case A_CPPCR:
        val &= R_CPPCR_VALID_MASK;
        /* If the PLL is enabled then set PLLS to say we have stabilized */
        val &= ~R_CPPCR_PLLS_MASK;
        if (val & R_CPPCR_PLLEN_MASK) {
            val |= R_CPPCR_PLLS_MASK;
        }
        s->regs[R_CPPCR] = val;
        break;
    case A_CPPCR1:
        val &= R_CPPCR1_VALID_MASK;
        /*
         * If the PLL is enabled then set PLL1S and PLLON to say we
         * are on and stabilized; otherwise set PLLOFF.
         */
        val &= ~(R_CPPCR1_PLL1S_MASK | R_CPPCR1_PLLOFF_MASK |
                 R_CPPCR1_PLLON_MASK);
        if (val & R_CPPCR1_PLL1EN_MASK) {
            val |= R_CPPCR1_PLL1S_MASK | R_CPPCR1_PLLON_MASK;
        } else {
            val |= R_CPPCR1_PLLOFF_MASK;
        }
        s->regs[R_CPPCR1] = val;
        break;
    case A_RSR:
        /* Writing 0 clears bits, writing 1 is ignored */
        s->regs[R_RSR] &= val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 CPM write: bad offset 0x%x\n", (uint32_t)addr);
        break;
    }
}

static const MemoryRegionOps jz4760_cpm_ops = {
    .read = jz4760_cpm_read,
    .write = jz4760_cpm_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void jz4760_cpm_reset(DeviceState *dev)
{
    JZ4760CPM *s = JZ4760_CPM(dev);

    memset(s->regs, 0, sizeof(s->regs));

    s->regs[R_CPCCR] = 0x01011100;
    s->regs[R_CPPCR] = 0x28080011;
    s->regs[R_CPPSR] = 0x80000000;
    s->regs[R_CPPCR1] = 0x28080002;
    s->regs[R_CPSPPR] = 0x0000a5a5;
    s->regs[R_USBPCR] = 0x42992198;
    s->regs[R_USBRDT] = 0x00000096;
    s->regs[R_USBVBFIL] = 0x00000080;
    s->regs[R_LCR] = 0x000000f8;
    s->regs[R_CLKGR0] = 0x3fffffe0;
    s->regs[R_OPCR] = 0x00001570;
    s->regs[R_CLKGR1] = 0x0000017f;
    s->regs[R_RSR] = 0x1; /* all our resets are power on resets */
}

static void jz4760_cpm_init(Object *obj)
{
    JZ4760CPM *s = JZ4760_CPM(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &jz4760_cpm_ops, s,
                          "jz4760-cpm", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void jz4760_cpm_realize(DeviceState *dev, Error **errp)
{
}

static const VMStateDescription jz4760_cpm_vmstate = {
    .name = "jz4760-cpm",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, JZ4760CPM, JZ4760_CPM_NUM_REGS),
        VMSTATE_END_OF_LIST()
    }
};

static void jz4760_cpm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_cpm_realize;
    dc->vmsd = &jz4760_cpm_vmstate;
    dc->reset = jz4760_cpm_reset;
}

static const TypeInfo jz4760_cpm_info = {
    .name = TYPE_JZ4760_CPM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760CPM),
    .instance_init = jz4760_cpm_init,
    .class_init = jz4760_cpm_class_init,
};

static void jz4760_cpm_register_types(void)
{
    type_register_static(&jz4760_cpm_info);
}

type_init(jz4760_cpm_register_types);
