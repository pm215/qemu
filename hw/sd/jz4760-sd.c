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
#include "hw/sd/jz4760-sd.h"

REG32(CTRL, 0x0)
    FIELD(CTRL, CLOCK_CONTROL, 0, 2)
    FIELD(CTRL, START_OP, 2, 1)
    FIELD(CTRL, RESET, 3, 1)
    FIELD(CTRL, STOP_READWAIT, 4, 1)
    FIELD(CTRL, START_READWAIT, 5, 1)
    FIELD(CTRL, EXIT_TRANSFER, 6, 1)
    FIELD(CTRL, EXIT_MULTIPLE, 7, 1)
    FIELD(CTRL, SEND_AS_CCSD, 14, 1)
    FIELD(CTRL, SEND_CCSD, 15, 1)
REG32(STAT, 0x4)
    FIELD(STAT, TIME_OUT_READ, 0, 1)
    FIELD(STAT, TIME_OUT_RES, 1, 1)
    FIELD(STAT, CRC_WRITE_ERROR, 2, 2)
    FIELD(STAT, CRC_READ_ERROR, 4, 1)
    FIELD(STAT, CRC_RES_ERR, 5, 1)
    FIELD(STAT, DATA_FIFO_EMPTY, 6, 1)
    FIELD(STAT, DATA_FIFO_FULL, 7, 1)
    FIELD(STAT, CLK_EN, 8, 1)
    FIELD(STAT, IS_READWAIT, 9, 1)
    FIELD(STAT, DATA_FIFO_AFULL, 10, 1)
    FIELD(STAT, END_CMD_RES, 11, 1)
    FIELD(STAT, DATA_TRAN_DONE, 12, 1)
    FIELD(STAT, PRG_DONE, 13, 1)
    FIELD(STAT, SDIO_INT_ACTIVE, 14, 1)
    FIELD(STAT, IS_RESETTING, 15, 1)
    FIELD(STAT, AUTO_CMD_DONE, 31, 1)
REG32(CLKRT, 0x8)
    FIELD(CLKRT, CLK_RATE, 0, 2)
REG32(CMDAT, 0xc)
    FIELD(CMDAT, RESPONSE_FORMAT, 0, 2)
    FIELD(CMDAT, DATA_EN, 3, 1)
    FIELD(CMDAT, WRITE_READ, 4, 1)
    FIELD(CMDAT, STREAM_BLOCK, 5, 1)
    FIELD(CMDAT, BUSY, 6, 1)
    FIELD(CMDAT, INIT, 7, 1)
    FIELD(CMDAT, DMA_EN, 8, 1)
    FIELD(CMDAT, BUS_WIDTH, 9, 2)
    FIELD(CMDAT, STOP_ABORT, 11, 1)
    FIELD(CMDAT, TTRG, 12, 2)
    FIELD(CMDAT, RTRG, 14, 2)
    FIELD(CMDAT, SEND_AS_STOP, 16, 1)
    FIELD(CMDAT, SDIO_PRDT, 17, 1)
    FIELD(CMDAT, READ_CEATA, 30, 1)
    FIELD(CMDAT, CCS_EXPECTED, 31, 1)
#define R_CMDAT_VALID_MASK (R_CMDAT_RESPONSE_FORMAT_MASK |      \
                            R_CMDAT_DATA_EN_MASK |              \
                            R_CMDAT_WRITE_READ_MASK |           \
                            R_CMDAT_STREAM_BLOCK_MASK |         \
                            R_CMDAT_BUSY_MASK |                 \
                            R_CMDAT_INIT_MASK |                 \
                            R_CMDAT_DMA_EN_MASK |               \
                            R_CMDAT_BUS_WIDTH_MASK |            \
                            R_CMDAT_STOP_ABORT_MASK |           \
                            R_CMDAT_TTRG_MASK |                 \
                            R_CMDAT_RTRG_MASK |                 \
                            R_CMDAT_SEND_AS_STOP_MASK |         \
                            R_CMDAT_SDIO_PRDT_MASK |            \
                            R_CMDAT_READ_CEATA_MASK |           \
                            R_CMDAT_CCS_EXPECTED_MASK)
REG32(RESTO, 0x10)
    FIELD(RESTO, RES_TO, 0, 8)
REG32(RDTO, 0x14)
REG32(BLKLEN, 0x18)
REG32(NOB, 0x1c)
REG32(SNOB, 0x20)
REG32(IMASK, 0x24)
REG32(IREG, 0x28)
    FIELD(IREG, DATA_TRAN_DONE, 0, 1)
    FIELD(IREG, PRG_DONE, 1, 1)
    FIELD(IREG, END_CMD_RES, 2, 1)
    FIELD(IREG, RXFIFO_RD_REQ, 5, 1)
    FIELD(IREG, TXFIFO_WR_REQ, 6, 1)
    FIELD(IREG, SDIO, 7, 1)
    FIELD(IREG, TIME_OUT_READ, 8, 1)
    FIELD(IREG, TIME_OUT_RES, 9, 1)
    FIELD(IREG, CRC_WRITE_ERR, 10, 1)
    FIELD(IREG, CRC_READ_ERR, 11, 1)
    FIELD(IREG, CRC_RES_ERR, 12, 1)
    FIELD(IREG, DATA_FIFO_EMP, 13, 1)
    FIELD(IREG, DATA_FIFO_FULL, 14, 1)
    FIELD(IREG, AUTO_CMD_DONE, 15, 1)
#define R_IREG_VALID_MASK (R_IREG_DATA_TRAN_DONE_MASK | \
                           R_IREG_PRG_DONE_MASK |       \
                           R_IREG_END_CMD_RES_MASK |    \
                           R_IREG_RXFIFO_RD_REQ_MASK |  \
                           R_IREG_TXFIFO_WR_REQ_MASK |  \
                           R_IREG_SDIO_MASK |           \
                           R_IREG_TIME_OUT_READ_MASK |  \
                           R_IREG_TIME_OUT_RES_MASK |   \
                           R_IREG_CRC_WRITE_ERR_MASK |  \
                           R_IREG_CRC_READ_ERR_MASK |   \
                           R_IREG_CRC_RES_ERR_MASK |    \
                           R_IREG_DATA_FIFO_EMP_MASK |  \
                           R_IREG_DATA_FIFO_FULL_MASK | \
                           R_IREG_AUTO_CMD_DONE_MASK)
REG32(CMD, 0x2c)
    FIELD(CMD, CMD_INDEX, 0, 6)
REG32(ARG, 0x30)
REG32(RES, 0x34)
REG32(RXFIFO, 0x38)
REG32(TXFIFO, 0x3c)
REG32(LPM, 0x40)
    FIELD(LPM, LPM, 0, 1)

static void jz4760_sd_reset(DeviceState *dev)
{
}

static int regwidth(hwaddr addr)
{
    switch (addr) {
    case A_CMD:
        return 1;
    case A_CTRL:
    case A_CLKRT:
    case A_RESTO:
    case A_BLKLEN:
    case A_NOB:
    case A_SNOB:
    case A_IREG:
    case A_RES:
        return 2;
    default:
        return 4;
    }
}

static uint64_t jz4760_sd_read(void *opaque, hwaddr addr, unsigned size)
{
    uint64_t r = 0;

    /*
     * Registers are varyingly 8, 16 or 32 bit. Ignore accesses with
     * the wrong width. (Unclear what the hardware does.)
     */
    if (size != regwidth(addr)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 SD read: bad size %d (expected %d) "
                      "for offset 0x%x\n",
                      size, regwidth(addr), (uint32_t)addr);
        goto out;
    }

    switch (addr) {
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 SD read: bad offset 0x%x\n", (uint32_t)addr);
        r = 0;
        break;
    }

out:
    trace_jz4760_sd_read(addr, r, size);
    return r;
}

static void jz4760_sd_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    trace_jz4760_sd_write(addr, val, size);

    if (size != regwidth(addr)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 SD write: bad size %d (expected %d) "
                      "for offset 0x%x\n",
                      size, regwidth(addr), (uint32_t)addr);
        return;
    }


    switch (addr) {
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 SD write: bad offset 0x%x\n", (uint32_t)addr);
        break;
    }
}

static const MemoryRegionOps jz4760_sd_ops = {
    .read = jz4760_sd_read,
    .write = jz4760_sd_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
};

static void jz4760_sd_set_inserted(DeviceState *dev, bool inserted)
{
}

static void jz4760_sd_set_readonly(DeviceState *dev, bool readonly)
{
}

static void jz4760_sd_init(Object *obj)
{
    JZ4760SD *s = JZ4760_SD(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    qbus_create_inplace(&s->sdbus, sizeof(s->sdbus),
                                          TYPE_JZ4760_SD_BUS,
                                          DEVICE(obj), "sd-bus");

    memory_region_init_io(&s->iomem, obj, &jz4760_sd_ops, s,
                          "jz4760-sd", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void jz4760_sd_realize(DeviceState *dev, Error **errp)
{
}

static const VMStateDescription jz4760_sd_vmstate = {
    .name = "jz4760-sd",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void jz4760_sd_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_sd_realize;
    dc->vmsd = &jz4760_sd_vmstate;
    dc->reset = jz4760_sd_reset;
}

static void jz4760_sd_bus_class_init(ObjectClass *klass, void *data)
{
    SDBusClass *sbc = SD_BUS_CLASS(klass);

    sbc->set_inserted = jz4760_sd_set_inserted;
    sbc->set_readonly = jz4760_sd_set_readonly;
}

static const TypeInfo jz4760_sd_info = {
    .name = TYPE_JZ4760_SD,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760SD),
    .instance_init = jz4760_sd_init,
    .class_init = jz4760_sd_class_init,
};

static const TypeInfo jz4760_sd_bus_info = {
    .name = TYPE_JZ4760_SD_BUS,
    .parent = TYPE_SD_BUS,
    .instance_size = sizeof(SDBus),
    .class_init = jz4760_sd_bus_class_init,
};

static void jz4760_sd_register_types(void)
{
    type_register_static(&jz4760_sd_info);
    type_register_static(&jz4760_sd_bus_info);
}

type_init(jz4760_sd_register_types);
