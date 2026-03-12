#include <zephyr/init.h>

#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_BLE) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#define BLE_PROFILE_1 1
#define DEFAULT_LAYER_1 1

static int apply_default_layer_for_profile(uint8_t profile_index) {
    int ret = zmk_keymap_layer_deactivate(DEFAULT_LAYER_1);

    if (ret < 0) {
        LOG_WRN("Failed to deactivate Mac layer %d: %d", DEFAULT_LAYER_1, ret);
    }

    if (profile_index != BLE_PROFILE_1) {
        LOG_INF("BLE profile %d selected, using Windows layer", profile_index);
        return 0;
    }

    ret = zmk_keymap_layer_activate(DEFAULT_LAYER_1);
    if (ret < 0) {
        LOG_WRN("Failed to activate Mac layer %d: %d", DEFAULT_LAYER_1, ret);
        return ret;
    }

    LOG_INF("BLE profile %d selected, using Mac layer", profile_index);
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