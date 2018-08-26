/*
 * JZ4760 Timer Counter Unit
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
#include "hw/timer/jz4760-tcu.h"

/*
 * The TCU combines several things:
 *
 * (1) the OST ("operating system timer"), which is a 32-bit upcounter
 * (2) the WDT ("watchdog timer"), which is a 16-bit upcounter
 * (3) 8 identical counter modules, each of which is a 16-bit upcounter
 */

/* Common registers -- offset is from base of the TCU register bank */
REG32(TSTR, 0xf0)
    FIELD(TSTR, BUSY1, 1, 1)
    FIELD(TSTR, BUSY2, 2, 1)
    FIELD(TSTR, REAL1, 17, 1)
    FIELD(TSTR, REAL2, 18, 1)
#define R_TSTR_VALID_MASK 0x00060006
REG32(TSTSR, 0xf4)
REG32(TSTCR, 0xf8)
REG32(TSR, 0x1c)
    FIELD(TSR, OSTS, 15, 1)
    FIELD(TSR, WDTS, 16, 1)
#define R_TSR_VALID_MASK 0x000180ff
REG32(TSSR, 0x2c)
REG32(TSCR, 0x3c)
REG32(TER, 0x10)
    FIELD(TER, OSTEN, 15, 1)
#define R_TER_VALID_MASK 0x80ff
REG32(TESR, 0x14)
REG32(TECR, 0x18)
REG32(TFR, 0x20)
    FIELD(TFR, OSTFLAG, 16, 1)
#define R_TFR_VALID_MASK 0x00ff80ff
REG32(TFSR, 0x24)
REG32(TFCR, 0x28)
REG32(TMR, 0x30)
    FIELD(TMR, OSTFLAG, 16, 1)
#define R_TMR_VALID_MASK 0x00ff80ff
REG32(TMSR, 0x34)
REG32(TMCR, 0x38)
REG32(OSTDR, 0xe0)
REG32(OSTCNT, 0xe8)
REG32(OSTCSR, 0xec)
    FIELD(OSTCSR, PCK_EN, 0, 1)
    FIELD(OSTCSR, RTC_EN, 1, 1)
    FIELD(OSTCSR, EXT_EN, 2, 1)
    FIELD(OSTCSR, PRESCALE, 3, 3)
    FIELD(OSTCSR, SD, 9, 1)
    FIELD(OSTCSR, CNT_MD, 15, 1)
#define R_OSTCSR_VALID_MASK 0x823f
/*
 * The data sheet doesn't use the WD prefix for the watchdog registers,
 * but we do to avoid a naming clash with the per-counter registers.
 */
REG32(WDTDR, 0x0)
REG32(WDTCER, 0x4)
    FIELD(WDTCER, TECN, 0, 1)
#define R_WDTCER_VALID_MASK 1
REG32(WDTCNT, 0x8)
REG32(WDTCSR, 0xc)
    FIELD(WDTCSR, PCK_EN, 0, 1)
    FIELD(WDTCSR, RTC_EN, 1, 1)
    FIELD(WDTCSR, EXT_EN, 2, 1)
    FIELD(WDTCSR, PRESCALE, 3, 3)
#define R_WDTCSR_VALID_MASK 0x3f

/*
 * Per-counter registers -- offset is from the base for that counter.
 * There are 8 counters, and their registers are at 0x40, 0x50, 0x60...
 * up to counter 7 at 0xb0.
 */
REG32(TDFR, 0x0)
REG32(TDHR, 0x4)
REG32(TCNT, 0x8)
REG32(TCSR, 0xc)
    FIELD(TCSR, PCK_EN, 0, 1)
    FIELD(TCSR, RTC_EN, 1, 1)
    FIELD(TCSR, EXT_EN, 2, 1)
    FIELD(TCSR, PRESCALE, 3, 3)
    FIELD(TCSR, PWM_IN_EN, 6, 1)
    FIELD(TCSR, PWM_EN, 7, 1)
    FIELD(TCSR, INTL, 8, 1)
    FIELD(TCSR, SD, 9, 1)
    FIELD(TCSR, CLRZ, 10, 1)
#define R_TCSR_VALID_MASK 0x7ff

/* each counter has:
 * an upcounter which starts at 0 and counts up
 * when it hits TDFR it resets to 0 and continues counting.
 *
 * TER has a bit per counter, which is a simple enable (count or don't)
 * It also has the OSTEN bit.
 * TFR has bits set to 1 for comparison matches:
 * for each counter, a bit for "counter hit TDHR value" and one for
 * "counter hit TDFR value"
 * also it has an OSTCNT == OSTDR match
 * TMR has a mask bit for each bit in TFR, which says "don't interrupt on this"
 * TSR has a "start/stop clock to each counter" bit, plus one for OST and
 * one for WDT.
 * TSTR is odd, it is for TCU2 mode.
 * We should just have this as REAL bit is always set, BUSY bit always clear,
 * since we don't have "counter currently busy" in our implementation.
 *
 * OST is the Operating System Timer:
 * it has a control register, a 32-bit upcounter and OSTDR has the comparison
 * value.
 *
 * WDT is the watchdog: a 16 bit upcounter, with a data register, a control
 * register and a simple enable.
 */

static int regwidth(hwaddr addr)
{
    switch (addr) {
    case A_WDTCER:
        return 1;
    case A_TSTR:
    case A_TSTSR:
    case A_TSTCR:
    case A_TSR:
    case A_TSSR:
    case A_TSCR:
    case A_TFR:
    case A_TFSR:
    case A_TFCR:
    case A_TMR:
    case A_TMSR:
    case A_TMCR:
    case A_OSTDR:
    case A_OSTCNT:
        return 4;
    default:
        return 2;
    }
}

static uint64_t jz4760_tcu_read(void *opaque, hwaddr addr, unsigned size)
{
    JZ4760TCU *s = opaque;
    uint64_t r = 0;

    if (size != regwidth(addr)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 TCU read: bad size %d (expected %d) "
                      "for offset 0x%x\n",
                      size, regwidth(addr), (uint32_t)addr);
        goto out;
    }

    switch (addr) {
    case 0x40 ... 0xbf:
    {
        int cidx = extract32(addr - 0x40, 4, 3);
        JZ4760TCUCounter *c = &s->counter[cidx];

        switch (extract32(addr, 0, 4)) {
        case A_TDFR:
            r = c->tdfr;
            break;
        case A_TDHR:
            r = c->tdhr;
            break;
        case A_TCNT:
            break;
        case A_TCSR:
            r = c->tcsr;
            break;
        default:
            g_assert_not_reached();
        }
        break;
    }
    case A_TSTR:
        /* Our counters always return the true value and are never busy */
        r = R_TSTR_REAL1_MASK | R_TSTR_REAL2_MASK;
        break;
    case A_TSR:
        r = s->tsr;
        break;
    case A_TER:
        r = s->ter;
        break;
    case A_TFR:
        r = s->tfr;
        break;
    case A_TMR:
        r = s->tmr;
        break;
    case A_OSTDR:
        r = s->ostdr;
        break;
    case A_OSTCNT:
        r = s->ostcnt;
        break;
    case A_OSTCSR:
        r = s->ostcsr;
        break;
    case A_WDTDR:
        r = s->wdtdr;
        break;
    case A_WDTCER:
        r = s->wdtcer;
        break;
    case A_WDTCNT:
        r = s->wdtcnt;
        break;
    case A_WDTCSR:
        r = s->wdtcsr;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 TCU read: bad offset 0x%x\n", (uint32_t)addr);
        r = 0;
        break;
    }

out:
    trace_jz4760_tcu_read(addr, r, size);
    return r;
}

static void jz4760_tcu_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    JZ4760TCU *s = opaque;
    trace_jz4760_tcu_write(addr, val, size);

    if (size != regwidth(addr)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 TCU write: bad size %d (expected %d) "
                      "for offset 0x%x\n",
                      size, regwidth(addr), (uint32_t)addr);
        return;
    }

    switch (addr) {
    case 0x40 ... 0xbf:
    {
        int cidx = extract32(addr - 0x40, 4, 3);
        JZ4760TCUCounter *c = &s->counter[cidx];

        switch (extract32(addr, 0, 4)) {
        case A_TDFR:
            c->tdfr = val;
            break;
        case A_TDHR:
            c->tdhr = val;
            break;
        case A_TCNT:
            break;
        case A_TCSR:
            c->tcsr = val & R_TCSR_VALID_MASK;
            break;
        default:
            g_assert_not_reached();
        }
        break;
    }
    case A_TSTSR:
    case A_TSTCR:
        /* Our TSTR is a fixed value, so set and clear do nothing */
        break;
    case A_TSSR:
        s->tsr |= (val & R_TSR_VALID_MASK);
        break;
    case A_TSCR:
        s->tsr &= ~(val & R_TSR_VALID_MASK);
        break;
    case A_TESR:
        s->ter |= (val & R_TER_VALID_MASK);
        break;
    case A_TECR:
        s->ter &= ~(val & R_TER_VALID_MASK);
        break;
    case A_TFSR:
        s->tfr |= (val & R_TFR_VALID_MASK);
        break;
    case A_TFCR:
        s->tfr &= ~(val & R_TFR_VALID_MASK);
        break;
    case A_TMSR:
        s->tmr |= (val & R_TMR_VALID_MASK);
        break;
    case A_TMCR:
        s->tmr &= ~(val & R_TMR_VALID_MASK);
        break;
    case A_OSTDR:
        s->ostdr = val;
        break;
    case A_OSTCNT:
        s->ostcnt = val;
        break;
    case A_OSTCSR:
        s->ostcsr = val & R_OSTCSR_VALID_MASK;
        break;
    case A_WDTDR:
        s->wdtdr = val;
        break;
    case A_WDTCER:
        s->wdtcer = val & R_WDTCER_VALID_MASK;
        break;
    case A_WDTCNT:
        s->wdtcnt = val;
        break;
    case A_WDTCSR:
        s->wdtcsr = val & R_WDTCSR_VALID_MASK;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "jz4760 TCU write: bad offset 0x%x\n", (uint32_t)addr);
        break;
    }
}

static const MemoryRegionOps jz4760_tcu_ops = {
    .read = jz4760_tcu_read,
    .write = jz4760_tcu_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void jz4760_tcu_reset(DeviceState *dev)
{
    JZ4760TCU *s = JZ4760_TCU(dev);
    int i;

    s->tsr = 0;
    s->ter = 0;
    s->tfr = 0x003f003f;
    s->tmr = 0;

    s->ostdr = 0;
    s->ostcnt = 0;
    s->ostcsr = 0;

    s->wdtdr = 0;
    s->wdtcer = 0;
    s->wdtcnt = 0;
    s->wdtcsr = 0;

    for (i = 0; i < ARRAY_SIZE(s->counter); i++) {
        s->counter[i].tdfr = 0;
        s->counter[i].tdhr = 0;
        s->counter[i].tcnt = 0;
        s->counter[i].tcsr = 0;
    }
}

static void jz4760_tcu_init(Object *obj)
{
    JZ4760TCU *s = JZ4760_TCU(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    int i;

    memory_region_init_io(&s->iomem, obj, &jz4760_tcu_ops, s,
                          "jz4760-tcu", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);

    for (i = 0; i < ARRAY_SIZE(s->irq); i++) {
        sysbus_init_irq(sbd, &s->irq[i]);
    }
}

static void jz4760_tcu_realize(DeviceState *dev, Error **errp)
{
}

static const VMStateDescription jz4760_tcu_counter_vmstate = {
    .name = "jz4760-tcu-counter",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT16(tdfr, JZ4760TCUCounter),
        VMSTATE_UINT16(tdhr, JZ4760TCUCounter),
        VMSTATE_UINT16(tcnt, JZ4760TCUCounter),
        VMSTATE_UINT16(tcsr, JZ4760TCUCounter),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription jz4760_tcu_vmstate = {
    .name = "jz4760-tcu",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT_ARRAY(counter, JZ4760TCU,
                             JZ4760TCU_NUM_COUNTERS,
                             1, jz4760_tcu_counter_vmstate,
                             JZ4760TCUCounter),
        VMSTATE_UINT32(tsr, JZ4760TCU),
        VMSTATE_UINT16(ter, JZ4760TCU),
        VMSTATE_UINT32(tfr, JZ4760TCU),
        VMSTATE_UINT32(tmr, JZ4760TCU),
        VMSTATE_UINT32(ostdr, JZ4760TCU),
        VMSTATE_UINT32(ostcnt, JZ4760TCU),
        VMSTATE_UINT16(ostcsr, JZ4760TCU),
        VMSTATE_UINT16(wdtdr, JZ4760TCU),
        VMSTATE_UINT8(wdtcer, JZ4760TCU),
        VMSTATE_UINT16(wdtcnt, JZ4760TCU),
        VMSTATE_UINT16(wdtcsr, JZ4760TCU),
        VMSTATE_END_OF_LIST()
    }
};

static void jz4760_tcu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = jz4760_tcu_realize;
    dc->vmsd = &jz4760_tcu_vmstate;
    dc->reset = jz4760_tcu_reset;
}

static const TypeInfo jz4760_tcu_info = {
    .name = TYPE_JZ4760_TCU,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4760TCU),
    .instance_init = jz4760_tcu_init,
    .class_init = jz4760_tcu_class_init,
};

static void jz4760_tcu_register_types(void)
{
    type_register_static(&jz4760_tcu_info);
}

type_init(jz4760_tcu_register_types);
