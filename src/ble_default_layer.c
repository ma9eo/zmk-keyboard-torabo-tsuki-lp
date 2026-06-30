#include <zephyr/init.h>

#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_BLE) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#define BLE_PROFILE_MAC 0
#define MAC_LAYER 1
#define WINDOWS_LAYER 2

static int apply_default_layer_for_profile(uint8_t profile_index) {
    // どちらの OS レイヤーも一旦無効化してから、プロファイルに応じて有効化する
    int ret = zmk_keymap_layer_deactivate(MAC_LAYER);
    if (ret < 0) {
        LOG_WRN("Failed to deactivate Mac layer %d: %d", MAC_LAYER, ret);
    }
    ret = zmk_keymap_layer_deactivate(WINDOWS_LAYER);
    if (ret < 0) {
        LOG_WRN("Failed to deactivate Windows layer %d: %d", WINDOWS_LAYER, ret);
    }

    if (profile_index == BLE_PROFILE_MAC) {
        ret = zmk_keymap_layer_activate(MAC_LAYER);
        if (ret < 0) {
            LOG_WRN("Failed to activate Mac layer %d: %d", MAC_LAYER, ret);
            return ret;
        }
        LOG_INF("BLE profile %d selected, using Mac layer", profile_index);
        return 0;
    }

    ret = zmk_keymap_layer_activate(WINDOWS_LAYER);
    if (ret < 0) {
        LOG_WRN("Failed to activate Windows layer %d: %d", WINDOWS_LAYER, ret);
        return ret;
    }

    LOG_INF("BLE profile %d selected, using Windows layer", profile_index);
    return 0;
}

static int ble_default_layer_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *event =
        as_zmk_ble_active_profile_changed(eh);

    if (!event) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    apply_default_layer_for_profile(event->index);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ble_default_layer, ble_default_layer_listener);
ZMK_SUBSCRIPTION(ble_default_layer, zmk_ble_active_profile_changed);

static int ble_default_layer_init(void) {
    return apply_default_layer_for_profile(zmk_ble_active_profile_index());
}

SYS_INIT(ble_default_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif