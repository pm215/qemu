/*
 * XDuoo X3 music player emulation
 *
 * Copyright (c) 2018 Linaro Limited
 * Written by Peter Maydell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 or
 *  (at your option) any later version.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/boards.h"
#include "sysemu/sysemu.h"
#include "sysemu/qtest.h"
#include "hw/misc/unimp.h"
#include "hw/devices.h"
#include "hw/loader.h"
#include "exec/address-spaces.h"
#include "hw/mips/jz4760.h"
#include "qemu/units.h"

typedef struct {
    MachineState parent;

    JZ4760 jz4760;
    MemoryRegion sram;
    MemoryRegion sram_alias;
} XDuooX3MachineState;

#define TYPE_XDUOO_X3_MACHINE MACHINE_TYPE_NAME("xduoo-x3")
#define XDUOO_X3_MACHINE(obj) \
    OBJECT_CHECK(XDuooX3MachineState, obj, TYPE_XDUOO_X3_MACHINE)

static void xduoo_x3_init(MachineState *machine)
{
    XDuooX3MachineState *xms = XDUOO_X3_MACHINE(machine);
    MemoryRegion *system_memory = get_system_memory();

    sysbus_init_child_obj(OBJECT(machine), "jz4760", &xms->jz4760,
                          sizeof(xms->jz4760), TYPE_JZ4760);
    object_property_set_link(OBJECT(&xms->jz4760), OBJECT(system_memory),
                             "memory", &error_fatal);
    object_property_set_bool(OBJECT(&xms->jz4760), true, "realized",
                             &error_fatal);

    memory_region_allocate_system_memory(&xms->sram,
                                         NULL, "sram", 32 * MiB);
    memory_region_add_subregion(system_memory, 0x20000000, &xms->sram);

    if (machine->firmware) {
        /*
         * While our JZ4760 model does not have a proper boot rom
         * implementation capable of loading the first 8K of NAND
         * flash into memory, we must load the binary into SDRAM
         * ourselves.
         */
        if (load_image_targphys(machine->firmware, 0x20000000,
                                32 * MiB) == -1) {
            error_printf("Unable to load firmware image '%s'\n",
                         machine->firmware);
            exit(1);
        }
    } else if (!qtest_enabled()) {
        error_printf("Please provide a -bios argument\n");
        exit(1);
    }
}

static void xduoo_x3_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "Xduoo X3 music player";
    mc->max_cpus = 1;
    mc->init = xduoo_x3_init;
}

static const TypeInfo xduoo_x3_info = {
    .name = TYPE_XDUOO_X3_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(XDuooX3MachineState),
    .class_init = xduoo_x3_class_init,
};

static void xduoo_x3_machine_init(void)
{
    type_register_static(&xduoo_x3_info);
}

type_init(xduoo_x3_machine_init);
