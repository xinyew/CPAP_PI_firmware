/*
 * Sensor manager — one sampling thread driving every sensor at the
 * budgeted rate (see README "Sampling-Rate Budget"):
 *
 *   tick = 10 ms (100 Hz)
 *   - PPG: fetch one sample per ready MAX30101 every tick (100 Hz)
 *   - FSR: all four SAADC channels every tick (100 Hz)
 *   - MS5611: conversions started on ticks 0 mod 4, read on the next
 *     tick (>= 5 ms later) -> 25 Hz pressure across all six
 *     concurrently; one cycle per second runs D2 (temperature) instead
 *   - SHT40: one blocking measurement per second
 *
 * A one-line summary with measured rates prints every second.
 */

#include "sensor_manager.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "ppg_reader.h"
#include "sht40_reader.h"
#include "../drivers/driver_fsr.h"
#include "../drivers/driver_ms5611.h"

LOG_MODULE_REGISTER(sensor_mgr, LOG_LEVEL_INF);

#define TICK_MS            10
#define TICKS_PER_SECOND   (1000 / TICK_MS)

/* FSR TIA: R_fsr = R_fb * VREF / (V_out - VREF), R_fb = 100k */
#define FSR_RFB_OHM        100000

struct system_sensor_data g_sensor_data;

static K_THREAD_STACK_DEFINE(sensor_stack, 4096);
static struct k_thread sensor_thread;

static int32_t fsr_resistance(int32_t vout_mv, int32_t vref_mv)
{
    int32_t delta = vout_mv - vref_mv;

    if (delta < 2) {
        return -1;  /* open sensor / at or below reference */
    }
    return (int32_t)(((int64_t)FSR_RFB_OHM * vref_mv) / delta);
}

static void print_summary(uint32_t seconds)
{
    struct system_sensor_data *d = &g_sensor_data;

    printk("[%4us] PPG", seconds);
    for (int i = 0; i < PPG_COUNT; i++) {
        if (d->ppg[i].valid) {
            printk(" %d:R%u/I%u/G%u", i + 1, d->ppg[i].red,
                   d->ppg[i].ir, d->ppg[i].green);
        } else {
            printk(" %d:--", i + 1);
        }
    }
    printk(" @%uHz\n", d->ppg_rate);

    printk("        FSR %d/%d/%d mV Vref %d mV R", d->ff_mv[0], d->ff_mv[1],
           d->ff_mv[2], d->vref_mv);
    for (int i = 0; i < 3; i++) {
        if (d->rfsr_ohm[i] < 0) {
            printk(" --");
        } else {
            printk(" %d", d->rfsr_ohm[i]);
        }
    }
    printk(" ohm @%uHz\n", d->fsr_rate);

    printk("        P(Pa)");
    for (int i = 0; i < 6; i++) {
        if (d->baro_ok_mask & BIT(i)) {
            printk(" %d", d->baro_pa[i]);
        } else {
            printk(" --");
        }
    }
    printk(" @%uHz", d->baro_rate);

    if (d->sht_ok) {
        printk(" | SHT %d.%02uC %u.%02u%%RH\n",
               d->sht_temp_c100 / 100,
               (unsigned)(d->sht_temp_c100 < 0 ? -d->sht_temp_c100 : d->sht_temp_c100) % 100,
               (unsigned)d->sht_rh_x100 / 100, (unsigned)d->sht_rh_x100 % 100);
    } else {
        printk(" | SHT --\n");
    }
}

static void sensor_thread_fn(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    struct system_sensor_data *d = &g_sensor_data;

    ppg_reader_init();
    d->sht_ok = (sht40_reader_init() == 0);
    drv_ms5611_init();
    d->baro_ok_mask = drv_ms5611_ok_mask();
    if (drv_fsr_init() < 0) {
        LOG_ERR("FSR ADC init failed");
    }

    uint32_t tick = 0;
    uint32_t seconds = 0;
    uint32_t ppg_cnt = 0, fsr_cnt = 0, baro_cnt = 0;
    bool baro_pending = false;

    while (1) {
        /* PPG + FSR every tick (100 Hz) */
        if (ppg_reader_read(d->ppg) > 0) {
            ppg_cnt++;
        }

        struct fsr_data fsr;

        if (drv_fsr_read(&fsr) == 0) {
            for (int i = 0; i < 3; i++) {
                d->ff_mv[i] = fsr.ff_mv[i];
                d->rfsr_ohm[i] = fsr_resistance(fsr.ff_mv[i], fsr.vref_mv);
            }
            d->vref_mv = fsr.vref_mv;
            fsr_cnt++;
        }

        /* MS5611: start on tick 0 mod 4, read on tick 1 mod 4 (10 ms
         * later > 4.6 ms conversion) -> 25 Hz. One cycle per second
         * converts temperature instead of pressure.
         */
        if (d->baro_ok_mask != 0) {
            uint32_t phase = tick % 4;

            if (phase == 0) {
                bool temp_cycle = (tick % TICKS_PER_SECOND) == 0;

                baro_pending = (drv_ms5611_start_conv(temp_cycle) == 0);
            } else if (phase == 1 && baro_pending) {
                if (drv_ms5611_finish_conv() == 0) {
                    for (int i = 0; i < 6; i++) {
                        drv_ms5611_get(i, &d->baro_pa[i],
                                       &d->baro_temp_c100[i]);
                    }
                    baro_cnt++;
                }
                baro_pending = false;
            }
        }

        /* SHT40 once per second (blocking ~8.3 ms) */
        if (d->sht_ok && (tick % TICKS_PER_SECOND) == 50) {
            sht40_reader_read(&d->sht_temp_c100, &d->sht_rh_x100);
        }

        tick++;
        if ((tick % TICKS_PER_SECOND) == 0) {
            seconds++;
            d->ppg_rate = ppg_cnt;
            d->fsr_rate = fsr_cnt;
            d->baro_rate = baro_cnt;
            ppg_cnt = fsr_cnt = baro_cnt = 0;
            print_summary(seconds);
        }

        k_msleep(TICK_MS);
    }
}

int sensor_manager_start(void)
{
    k_thread_create(&sensor_thread, sensor_stack,
                    K_THREAD_STACK_SIZEOF(sensor_stack),
                    sensor_thread_fn, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    k_thread_name_set(&sensor_thread, "sensors");
    return 0;
}
