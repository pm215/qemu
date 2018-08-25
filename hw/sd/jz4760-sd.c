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
    JZ4760SD *s = JZ4760_SD(dev);

    s->stat = 0x40;
    s->clkrt = 0;
    s->cmdat = 0;
    s->resto = 0x40;
    s->rdto = 0xffff;
    s->blklen = 0;
    s->nob = 0;
    s->snob = 0;
    s->imask = 0xff;
    s->ireg = 0;
    s->arg = 0;
    s->lpm = 0;

    memset(&s->response, 0, sizeof(s->response));
    s->response_read_ptr = 0;
    s->response_len = 0;

    fifo32_reset(&s->datafifo);
}

static void jz4760_sd_recalc_ireg(JZ4760SD *s)
{
    /*
     * Set bits in IREG according to current status.
     * Unfortunately the bit order in STAT doesn't line up with IREG
     * TODO suspect this is wrong and really the IREG bit should
     * only be set when the condition becomes true.
     * XXX when does the STAT bit clear ?
     */
    if (s->stat & R_STAT_AUTO_CMD_DONE_MASK) {
        s->ireg |= R_IREG_AUTO_CMD_DONE_MASK;
    }
    if (s->stat & R_STAT_DATA_FIFO_FULL_MASK) {
        s->ireg |= R_IREG_DATA_FIFO_FULL_MASK;
    }
    if (s->stat & R_STAT_DATA_FIFO_EMPTY_MASK) {
        s->ireg |= R_IREG_DATA_FIFO_EMP_MASK;
    }
    if (s->stat & R_STAT_CRC_RES_ERR_MASK) {
        s->ireg |= R_IREG_CRC_RES_ERR_MASK;
    }
    if (s->stat & R_STAT_CRC_READ_ERROR_MASK) {
        s->ireg |= R_IREG_CRC_READ_ERR_MASK;
    }
    if (s->stat & R_STAT_CRC_WRITE_ERROR_MASK) {
        s->ireg |= R_IREG_CRC_WRITE_ERR_MASK;
    }
    if (s->stat & R_STAT_TIME_OUT_RES_MASK) {
        s->ireg |= R_IREG_TIME_OUT_RES_MASK;
    }
    if (s->stat & R_STAT_TIME_OUT_READ_MASK) {
        s->ireg |= R_IREG_TIME_OUT_READ_MASK;
    }
    if (s->stat & R_STAT_SDIO_INT_ACTIVE_MASK) {
        s->ireg |= R_IREG_SDIO_MASK;
    }
    if (s->stat & R_STAT_END_CMD_RES_MASK) {
        s->ireg |= R_IREG_END_CMD_RES_MASK;
    }
    if (s->stat & R_STAT_PRG_DONE_MASK) {
        s->ireg |= R_IREG_PRG_DONE_MASK;
    }
    if (s->stat & R_STAT_DATA_TRAN_DONE_MASK) {
        s->ireg |= R_IREG_DATA_TRAN_DONE_MASK;
    }
    // TODO TXFIFO_WR_REG and RXFIFO_RD_REQ
}

static void jz4760_sd_irq_update(JZ4760SD *s)
{
    bool level = s->ireg & ~s->imask;

    qemu_set_irq(s->irq, level);
}

static void jz4760_sd_send_command(JZ4760SD *s)
{
    /* Send command to the SD card */
    SDRequest request;

    request.cmd = s->cmd;
    request.arg = s->arg;

    s->stat &= ~R_STAT_DATA_TRAN_DONE_MASK;
    s->stat &= ~R_STAT_PRG_DONE_MASK;

    if (s->cmdat & R_CMDAT_DATA_EN_MASK) {
        fifo32_reset(&s->datafifo);
        s->stat &= ~(R_STAT_DATA_FIFO_FULL_MASK | R_STAT_DATA_FIFO_AFULL_MASK);
        s->stat |= R_STAT_DATA_FIFO_EMPTY_MASK;
    }

    /*
     * The RSP FIFO gets a fairly "raw" view of the response:
     * an R1 response includes
     * leading 0 start and transmission bits, 6 bits of cmd index,
     * then the 32 bits of status, and then 8 bits of ignored
     * which appear in the FIFO as [47:32], [31:16], [15:0].
     * TODO not completely clear whether the low 8 bits of actual
     * status go in [15:8] of the last halfword or [7:0].
     * For an R2 response, which is 136 bits on the wire,
     * the fifo has bits [135:8] of the response, and [7:0] (crc?) are dropped
     * so it will need 8 lots of 16 bit reads.
     *
     * sdbus_do_command() provides us with a slightly more "cooked" view:
     * R1 responses are written as the 4 status bytes into the response buffer
     * R2 responses are 16 bytes, including the CRC and the end bit but
     * not the start/reserved bits.
     * So we get sdbus_do_command() to start at byte 1 in the buffer,
     * leaving byte 0 for the start/transmission/command fields.
     */
    s->response_len = sdbus_do_command(&s->sdbus, &request, s->response + 1);

    // TODO there are probably status bits to handle here

    switch (s->response_len) {
    case 0:
        break;
    case 4:
        s->response[0] = s->cmd;
        s->response[5] = 0;
        s->response_len += 2;
        break;
    case 16:
        s->response[0] = 0x3f;
        break;
    default:
        g_assert_not_reached();
    }

    s->stat |= R_STAT_END_CMD_RES_MASK;

    jz4760_sd_recalc_ireg(s);
    jz4760_sd_irq_update(s);
}

static void jz4760_sd_run_fifo(JZ4760SD *s)
{
    /*
     * The guest has just either pushed data into (TX) or read data
     * from (RX) the data FIFO. Handle this by either sending the
     * data to the card or reading more data from the card to refill.
     */
    // TODO
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
    JZ4760SD *s = opaque;
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
    case A_STAT:
        r = s->stat;
        break;
    case A_CLKRT:
        r = s->clkrt;
        break;
    case A_CMDAT:
        r = s->cmdat;
        break;
    case A_RESTO:
        r = s->resto;
        break;
    case A_RDTO:
        r = s->rdto;
        break;
    case A_BLKLEN:
        r = s->blklen;
        break;
    case A_NOB:
        r = s->nob;
        break;
    case A_SNOB:
        r = s->snob;
        break;
    case A_IMASK:
        r = s->imask;
        break;
    case A_IREG:
        r = s->ireg;
        break;
    case A_CMD:
        r = s->cmd;
        break;
    case A_ARG:
        r = s->arg;
        break;
    case A_RES:
        if (s->response_read_ptr >= s->response_len - 1 ||
            s->response_len >= ARRAY_SIZE(s->response)) {
            r = 0;
        } else {
            r = s->response[s->response_read_ptr++] << 8;
            r |= s->response[s->response_read_ptr++];
        }
        break;
    case A_RXFIFO:
        if (fifo32_is_empty(&s->datafifo)) {
            qemu_log_mask(LOG_GUEST_ERROR, "jz4760 SD: RXFIFO overrun\n");
            r = 0;
        } else {
            r = fifo32_pop(&s->datafifo);
            jz4760_sd_run_fifo(s);
        }
        break;
    case A_LPM:
        r = s->lpm;
        break;
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
    JZ4760SD *s = opaque;
    trace_jz4760_sd_write(addr, val, size);

    if (size != regwidth(addr)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 SD write: bad size %d (expected %d) "
                      "for offset 0x%x\n",
                      size, regwidth(addr), (uint32_t)addr);
        return;
    }

    switch (addr) {
    case A_CTRL:
        /* Write-only register with various "now do this" bits */
        if (val & R_CTRL_RESET_MASK) {
            jz4760_sd_reset(DEVICE(s));
            jz4760_sd_irq_update(s);
            /* Assume that reset is the only thing that happens */
            break;
        }
        /* All we do for clock control is remember if clock is on or off */
        switch (FIELD_EX32(val, CTRL, CLOCK_CONTROL)) {
        case 1:
            s->stat &= ~R_STAT_CLK_EN_MASK;
            break;
        case 2:
            s->stat |= R_STAT_CLK_EN_MASK;
            break;
        default:
            break;
        }
        if (val & R_CTRL_START_OP_MASK) {
            jz4760_sd_send_command(s);
        }
        // TODO other bits
        break;
    case A_CLKRT:
        s->clkrt = val & R_CLKRT_CLK_RATE_MASK;
        break;
    case A_CMDAT:
        s->cmdat = val & R_CMDAT_VALID_MASK;
        if (!(s->cmdat & R_CMDAT_DMA_EN_MASK)) {
            s->ireg &= ~(R_IREG_RXFIFO_RD_REQ_MASK | R_IREG_TXFIFO_WR_REQ_MASK);
            jz4760_sd_irq_update(s);
        }
        break;
    case A_RESTO:
        s->resto = val & R_RESTO_RES_TO_MASK;
        break;
    case A_RDTO:
        s->rdto = val;
        break;
    case A_BLKLEN:
        s->blklen = val;
        break;
    case A_NOB:
        s->nob = val;
        break;
    case A_IMASK:
        s->imask = val & R_IREG_VALID_MASK;
        jz4760_sd_irq_update(s);
        break;
    case A_IREG:
        /* Write-one-to-clear */
        s->imask &= ~(val & R_IREG_VALID_MASK);
        jz4760_sd_irq_update(s);
        break;
    case A_CMD:
        s->cmd = val & R_CMD_CMD_INDEX_MASK;
        break;
    case A_ARG:
        s->arg = val;
        break;
    case A_TXFIFO:
        if (fifo32_is_full(&s->datafifo)) {
            qemu_log_mask(LOG_GUEST_ERROR, "jz4760 SD: TXFIFO overrun\n");
        } else {
            fifo32_push(&s->datafifo, val);
            jz4760_sd_run_fifo(s);
        }
        break;
    case A_LPM:
        s->lpm = val & R_LPM_LPM_MASK;
        break;
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
    JZ4760SD *s = JZ4760_SD(dev);

    fifo32_create(&s->datafifo, 16);
}

static const VMStateDescription jz4760_sd_vmstate = {
    .name = "jz4760-sd",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(stat, JZ4760SD),
        VMSTATE_UINT32(clkrt, JZ4760SD),
        VMSTATE_UINT32(cmdat, JZ4760SD),
        VMSTATE_UINT32(resto, JZ4760SD),
        VMSTATE_UINT32(rdto, JZ4760SD),
        VMSTATE_UINT32(blklen, JZ4760SD),
        VMSTATE_UINT32(nob, JZ4760SD),
        VMSTATE_UINT32(snob, JZ4760SD),
        VMSTATE_UINT32(imask, JZ4760SD),
        VMSTATE_UINT32(ireg, JZ4760SD),
        VMSTATE_UINT32(cmd, JZ4760SD),
        VMSTATE_UINT32(arg, JZ4760SD),
        VMSTATE_UINT32(lpm, JZ4760SD),
        VMSTATE_UINT8_ARRAY(response, JZ4760SD, 17),
        VMSTATE_UINT32(response_read_ptr, JZ4760SD),
        VMSTATE_UINT32(response_len, JZ4760SD),
        VMSTATE_FIFO32(datafifo, JZ4760SD),
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
