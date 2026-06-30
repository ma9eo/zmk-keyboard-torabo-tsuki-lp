/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_profile_morph

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/ble.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// BLE プロファイルで bindings[0]=Mac / bindings[1]=Windows を出力し分ける behavior。
// 単一のマウスジェスチャプロセッサを base チェーンに置いたまま（suppress-movement が
// 効く）OS ごとに異なるキーを送るために使う。Mac は mac-profile（既定 0）で判定。
struct behavior_profile_morph_config {
    struct zmk_behavior_binding mac_binding;
    struct zmk_behavior_binding win_binding;
    uint8_t mac_profile;
};

struct behavior_profile_morph_data {
    struct zmk_behavior_binding *pressed_binding;
};

static int on_profile_morph_binding_pressed(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_profile_morph_config *cfg = dev->config;
    struct behavior_profile_morph_data *data = dev->data;

    if (data->pressed_binding != NULL) {
        LOG_ERR("Can't press the same profile-morph twice");
        return -ENOTSUP;
    }

    if (zmk_ble_active_profile_index() == cfg->mac_profile) {
        data->pressed_binding = (struct zmk_behavior_binding *)&cfg->mac_binding;
    } else {
        data->pressed_binding = (struct zmk_behavior_binding *)&cfg->win_binding;
    }
    return zmk_behavior_invoke_binding(data->pressed_binding, event, true);
}

static int on_profile_morph_binding_released(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_profile_morph_data *data = dev->data;

    if (data->pressed_binding == NULL) {
        LOG_ERR("Profile-morph already released");
        return -ENOTSUP;
    }

    struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
    data->pressed_binding = NULL;
    return zmk_behavior_invoke_binding(pressed_binding, event, false);
}

static const struct behavior_driver_api behavior_profile_morph_driver_api = {
    .binding_pressed = on_profile_morph_binding_pressed,
    .binding_released = on_profile_morph_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),               \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define KP_INST(n)                                                                                 \
    static struct behavior_profile_morph_config behavior_profile_morph_config_##n = {              \
        .mac_binding = _TRANSFORM_ENTRY(0, n),                                                      \
        .win_binding = _TRANSFORM_ENTRY(1, n),                                                      \
        .mac_profile = DT_INST_PROP_OR(n, mac_profile, 0),                                          \
    };                                                                                             \
    static struct behavior_profile_morph_data behavior_profile_morph_data_##n = {};                \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &behavior_profile_morph_data_##n,                        \
                            &behavior_profile_morph_config_##n, POST_KERNEL,                        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                            &behavior_profile_morph_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
