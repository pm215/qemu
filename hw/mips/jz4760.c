/*
 * JZ4760 MIPS SoC
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
#include "qemu/units.h"
#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "hw/mips/jz4760.h"
#include "hw/misc/unimp.h"
#include "hw/loader.h"
#include "hw/mips/cpudevs.h"
#include "hw/char/serial.h"

#define BOOTROM_BASE 0x1fc00000

static void gen_boot_rom(JZ4760 *s)
{
    /*
     * In an ideal world this would do what the real hardware bootrom does,
     * i.e. fish the first 8K out of the NAND flash and jump to it.
     * For now we rely on the board code having loaded the code at the
     * right place and just jump straight to 0x80000000.
     * We're assuming little-endian MIPS here...
     * The cache insn is QEMU specific magic to enable the "execute from
     * dcache/icache hard-wired in" trick. We borrow the "code 3" impdef
     * space, which is what the real jz4760 apparently uses for "write
     * specific data directly into the icache".
     */
    static const uint32_t bootrom[] = {
        0xbc030000, /* cache impdef */
        0x3c198000, /* lui $25, hi(0x800000A0) */
        0x373900A0, /* ori $25, lo(0x800000A0) */
        0x03200009, /* jr $25 */
        0x00000000, /* delay slot nop */
    };
    rom_add_blob_fixed_as("jz4760.bootrom", bootrom, sizeof(bootrom),
                          BOOTROM_BASE, CPU(s->cpu)->as);
}

static void jz4760_init(Object *obj)
{
    JZ4760 *s = JZ4760(obj);

    memory_region_init(&s->container, obj, "iotkit-container", UINT64_MAX);

    s->cpu = MIPS_CPU(object_new(MIPS_CPU_TYPE_NAME("jz4760")));

    sysbus_init_child_obj(obj, "intc", &s->intc, sizeof(s->intc),
                          TYPE_JZ4760_INTC);
    sysbus_init_child_obj(obj, "cpm", &s->cpm, sizeof(s->cpm),
                          TYPE_JZ4760_CPM);

    sysbus_init_child_obj(obj, "mdmac", &s->mdmac, sizeof(s->mdmac),
                          TYPE_JZ4760_DMA);
    sysbus_init_child_obj(obj, "dmac", &s->dmac, sizeof(s->dmac),
                          TYPE_JZ4760_DMA);
    sysbus_init_child_obj(obj, "bdmac", &s->bdmac, sizeof(s->bdmac),
                          TYPE_JZ4760_DMA);
    sysbus_init_child_obj(obj, "nemc", &s->nemc, sizeof(s->nemc),
                          TYPE_JZ4760_NEMC);
    object_property_add_alias(obj, "nand",
                              OBJECT(&s->nemc), "nand", &error_abort);
}

static void main_cpu_reset(void *opaque)
{
    MIPSCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}

static void jz4760_realize(DeviceState *dev, Error **errp)
{
    JZ4760 *s = JZ4760(dev);
    Error *err = NULL;
    MemoryRegion *mr;
    int i;

    if (!s->board_memory) {
        error_setg(errp, "memory property was not set");
        return;
    }

    memory_region_add_subregion_overlap(&s->container, 0, s->board_memory, -1);

    object_property_set_link(OBJECT(s->cpu), OBJECT(&s->container),
                             "memory", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_bool(OBJECT(s->cpu), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    cpu_mips_irq_init_cpu(s->cpu);
    cpu_mips_clock_init(s->cpu);
    qemu_register_reset(main_cpu_reset, s->cpu);

    memory_region_init_rom(&s->bootrom, OBJECT(s),
                           "jz4760.bootrom", 8 * KiB, &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    memory_region_add_subregion(&s->container, BOOTROM_BASE, &s->bootrom);
    gen_boot_rom(s);

    /*
     * We don't implement the SoC's support for changing the SRAM base,
     * so the alias at address 0 always points at 0x20000000.
     */
    memory_region_init_alias(&s->sram_alias, OBJECT(s), "sram-alias",
                             s->board_memory, 0x20000000, 256 * MiB);
    memory_region_add_subregion(&s->container, 0x00000000, &s->sram_alias);

    /* APB bus devices */

    /* CPM */
    object_property_set_bool(OBJECT(&s->cpm), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->cpm), 0);
    memory_region_add_subregion(&s->container, 0x10000000, mr);

    /* INTC */
    object_property_set_bool(OBJECT(&s->intc), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->intc), 0);
    memory_region_add_subregion(&s->container, 0x10001000, mr);
    /* MIPS CPU INT0 */
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->intc), 0, s->cpu->env.irq[2]);

    create_unimplemented_device("TCU",    0x10002000, 0x1000);
    create_unimplemented_device("RTC",    0x10003000, 0x1000);
    create_unimplemented_device("GPIO",   0x10010000, 0x1000);
    create_unimplemented_device("AIC",    0x10020000, 0x1000);
    create_unimplemented_device("MSC0",   0x10021000, 0x1000);
    create_unimplemented_device("MSC1",   0x10022000, 0x1000);
    create_unimplemented_device("MSC2",   0x10023000, 0x1000);

    /* UART0, UART1, UART2, UART3 */
    for (i = 0; i < 4; i++) {
        hwaddr base = 0x10030000 + 0x1000 * i;
        int uartirq = 5 - i;

        serial_mm_init(&s->container, base, 2,
                       qdev_get_gpio_in(DEVICE(&s->intc), uartirq),
                       115200, serial_hd(i), DEVICE_LITTLE_ENDIAN);
        /*
         * The JZ4760 UARTs are 16550 compatible but have extra
         * registers after the usual set. Stub those out for now.
         */
        create_unimplemented_device("UART0 extras", base + 0x20, 0x10);
    }

    create_unimplemented_device("SCC",    0x10040000, 0x1000);
    create_unimplemented_device("SSI0",   0x10043000, 0x1000);
    create_unimplemented_device("SSI1",   0x10044000, 0x1000);
    create_unimplemented_device("SSI2",   0x10045000, 0x1000);
    create_unimplemented_device("I2C0",   0x10050000, 0x1000);
    create_unimplemented_device("I2C1",   0x10051000, 0x1000);
    create_unimplemented_device("PS2",    0x10060000, 0x1000);
    create_unimplemented_device("SADC",   0x10070000, 0x1000);
    create_unimplemented_device("OWI",    0x10072000, 0x1000);
    create_unimplemented_device("TSSI",   0x10073000, 0x1000);

    /* AHB0 bus devices */
    create_unimplemented_device("HARB0",  0x13000000, 0x10000);
    create_unimplemented_device("EMC",    0x13010000, 0x10000);
    create_unimplemented_device("DDRC",   0x13020000, 0x10000);

    /* MDMAC */
    object_property_set_uint(OBJECT(&s->mdmac), 2, "num-channels", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_link(OBJECT(&s->mdmac), OBJECT(&s->container),
                             "downstream", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_bool(OBJECT(&s->mdmac), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->mdmac), 0);
    memory_region_add_subregion(&s->container, 0x13030000, mr);

    create_unimplemented_device("LCDC",   0x13050000, 0x10000);
    create_unimplemented_device("CIM",    0x13060000, 0x10000);
    /* The AOSD (on-screen display) device is not listed in the data sheet... */
    create_unimplemented_device("AOSD",   0x13070000, 0x10000);
    create_unimplemented_device("IPU",    0x13080000, 0x10000);

    /* AHB1 bus devices */
    create_unimplemented_device("HARB1",  0x13200000, 0x10000);
    create_unimplemented_device("DMAGP0", 0x13210000, 0x10000);
    create_unimplemented_device("DMAGP1", 0x13220000, 0x10000);
    create_unimplemented_device("DMAGP2", 0x13230000, 0x10000);
    create_unimplemented_device("MC",     0x13250000, 0x10000);
    create_unimplemented_device("ME",     0x13260000, 0x10000);
    create_unimplemented_device("DEBLK",  0x13270000, 0x10000);
    create_unimplemented_device("IDCT",   0x13280000, 0x10000);
    create_unimplemented_device("CABAC",  0x13290000, 0x10000);
    create_unimplemented_device("TCSM0",  0x132B0000, 0x10000);
    create_unimplemented_device("TCSM1",  0x132C0000, 0x10000);
    create_unimplemented_device("SRAM",   0x132D0000, 0x10000);

    /* AHB2 bus devices */
    create_unimplemented_device("HARB2",  0x13400000, 0x10000);

    /* NEMC */
    object_property_set_bool(OBJECT(&s->nemc), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    /* Registers: */
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->nemc), 0);
    memory_region_add_subregion(&s->container, 0x13410000, mr);
    /* NAND access region */
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->nemc), 1);
    memory_region_add_subregion(&s->container, 0x14000000, mr);

    /* DMAC */
    object_property_set_uint(OBJECT(&s->dmac), 2, "num-cores", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_uint(OBJECT(&s->dmac), 5, "num-channels", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_link(OBJECT(&s->dmac), OBJECT(&s->container),
                             "downstream", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_bool(OBJECT(&s->dmac), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->dmac), 0);
    memory_region_add_subregion(&s->container, 0x13420000, mr);

    create_unimplemented_device("UHC",    0x13430000, 0x10000);
    create_unimplemented_device("EDC",    0x13440000, 0x10000);

    /* BDMAC */
    object_property_set_uint(OBJECT(&s->bdmac), 3, "num-channels", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_link(OBJECT(&s->bdmac), OBJECT(&s->container),
                             "downstream", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_bool(OBJECT(&s->bdmac), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->bdmac), 0);
    memory_region_add_subregion(&s->container, 0x13450000, mr);

    create_unimplemented_device("GPS",    0x13480000, 0x10000);
    create_unimplemented_device("ETHC",   0x134B0000, 0x10000);
    create_unimplemented_device("BCH",    0x134D0000, 0x10000);
}

static Property jz4760_properties[] = {
    DEFINE_PROP_LINK("memory", JZ4760, board_memory, TYPE_MEMORY_REGION,
                     MemoryRegion *),
    DEFINE_PROP_END_OF_LIST()
};

static void jz4760_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_realize;
    dc->props = jz4760_properties;
}

static const TypeInfo jz4760_info = {
    .name = TYPE_JZ4760,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760),
    .instance_init = jz4760_init,
    .class_init = jz4760_class_init,
};

static void jz4760_register_types(void)
{
    type_register_static(&jz4760_info);
}

type_init(jz4760_register_types);
