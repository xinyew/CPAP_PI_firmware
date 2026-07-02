/*
 * BLE interface — GATT server for sensor sampling control.
 *
 * Advertises as "CPAP_PI_Control", exposes one Write-Without-Response
 * characteristic (16-bit sampling interval in ms, little-endian).
 * On write: validates range (10–10000 ms) and updates the interval
 * used by the main sampling loop.
 */

#include "ble_interface.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(ble_if, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------- */
/*  UUIDs                                                                     */
/* -------------------------------------------------------------------------- */

/* 16-bit custom vendor UUIDs */
#define BT_UUID_CPAP_PI_SVC_VAL       0xFFE0
#define BT_UUID_CPAP_PI_INTERVAL_VAL  0xFFE1

#define BT_UUID_CPAP_PI_SVC      BT_UUID_DECLARE_16(BT_UUID_CPAP_PI_SVC_VAL)
#define BT_UUID_CPAP_PI_INTERVAL BT_UUID_DECLARE_16(BT_UUID_CPAP_PI_INTERVAL_VAL)

/* -------------------------------------------------------------------------- */
/*  Advertising data                                                          */
/* -------------------------------------------------------------------------- */

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "CPAP_PI_Control", sizeof("CPAP_PI_Control") - 1),
};

/* -------------------------------------------------------------------------- */
/*  GATT characteristic — sampling interval                                   */
/* -------------------------------------------------------------------------- */

static uint16_t sample_interval_ms = 200;

uint16_t ble_interface_get_sample_interval_ms(void)
{
    return sample_interval_ms;
}

static ssize_t on_interval_write(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 const void *buf, uint16_t len,
                                 uint16_t offset, uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    /* Must be exactly 2 bytes, no offset */
    if (len != 2 || offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    /* Little-endian 16-bit unpack (no sys_ helper needed) */
    const uint8_t *p = (const uint8_t *)buf;
    uint16_t ms = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    if (ms < 10 || ms > 10000) {
        LOG_WRN("BLE: interval %u ms out of range", ms);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    sample_interval_ms = ms;
    LOG_INF("BLE: sample interval set to %u ms", ms);

    return len;
}

/* -------------------------------------------------------------------------- */
/*  GATT service definition                                                   */
/* -------------------------------------------------------------------------- */

BT_GATT_SERVICE_DEFINE(cpap_pi_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_CPAP_PI_SVC),
    BT_GATT_CHARACTERISTIC(BT_UUID_CPAP_PI_INTERVAL,
        BT_GATT_CHRC_WRITE_WITHOUT_RESP,
        BT_GATT_PERM_WRITE,
        NULL, on_interval_write, NULL),
);

/* -------------------------------------------------------------------------- */
/*  Connection callbacks                                                      */
/* -------------------------------------------------------------------------- */

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("BLE connection failed: %u", err);
        return;
    }

    LOG_INF("BLE connected");

    /* Request low-latency connection params (7.5 ms interval) */
    struct bt_le_conn_param param = {
        .interval_min = 6,
        .interval_max = 12,
        .latency = 0,
        .timeout = 400,
    };
    bt_conn_le_param_update(conn, &param);
}

static struct bt_le_adv_param adv_param;  /* saved for re-advertise */

static void restart_advertise(struct k_work *work)
{
    ARG_UNUSED(work);
    int ret = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (ret) {
        LOG_ERR("BLE re-advertise failed: %d", ret);
    } else {
        LOG_INF("BLE re-advertising");
    }
}

static K_WORK_DELAYABLE_DEFINE(adv_restart_work, restart_advertise);

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    LOG_INF("BLE disconnected (reason %u)", reason);

    /* Defer re-advertise — bt_le_adv_start must not be called
     * synchronously from the BLE stack's own callback context.
     */
    k_work_schedule(&adv_restart_work, K_MSEC(50));
}

static struct bt_conn_cb conn_callbacks = {
    .connected    = connected,
    .disconnected = disconnected,
};

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

int ble_interface_init(void)
{
    int ret;

    ret = bt_enable(NULL);
    if (ret) {
        LOG_ERR("BLE enable failed: %d", ret);
        return ret;
    }

    LOG_INF("BLE stack initialised");

    /* Register connection callbacks */
    bt_conn_cb_register(&conn_callbacks);

    /* Start connectable advertising — fast interval for quick discovery */
    adv_param = (struct bt_le_adv_param)BT_LE_ADV_PARAM_INIT(
        BT_LE_ADV_OPT_CONN,
        BT_GAP_ADV_FAST_INT_MIN_1,
        BT_GAP_ADV_FAST_INT_MAX_1,
        NULL);
    ret = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (ret) {
        LOG_ERR("BLE advertising start failed: %d", ret);
        return ret;
    }

    LOG_INF("BLE advertising as \"CPAP_PI_Control\"");
    return 0;
}
