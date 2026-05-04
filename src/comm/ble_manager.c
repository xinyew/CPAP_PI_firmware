#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/gap.h>
#include <bluetooth/services/nus.h>
#include <zephyr/logging/log.h>

#include "ble_manager.h"

LOG_MODULE_REGISTER(ble_manager, LOG_LEVEL_INF);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
    } else {
        LOG_INF("Connected");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");

    // Use explicit parameter structure for maximum compatibility across Zephyr versions
    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
        .peer = NULL,
    };

    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising started");
}

int ble_manager_init(void)
{
    int err;

    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    return 0;
}

int ble_manager_send(const char *data, size_t len)
{
    return bt_nus_send(NULL, (const uint8_t *)data, len);
}
