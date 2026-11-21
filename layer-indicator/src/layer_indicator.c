/*
 * 레이어가 바뀌면 지금 가장 위에 켜진 레이어를 F16~F19 키 한 번으로 컴퓨터에 알린다.
 *   레이어 0 → F16, 1 → F17, 2 → F18, 3 → F19
 * 맥의 Hammerspoon 스크립트가 이 키를 받아 표시하고, 다른 앱에는 전달하지 않는다.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <dt-bindings/zmk/keys.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const uint32_t layer_keys[] = {F16, F17, F18, F19};

static void send_layer(struct k_work *work) {
    uint8_t layer = zmk_keymap_highest_layer_active();
    if (layer >= ARRAY_SIZE(layer_keys)) {
        return;
    }
    int64_t now = k_uptime_get();
    raise_zmk_keycode_state_changed_from_encoded(layer_keys[layer], true, now);
    raise_zmk_keycode_state_changed_from_encoded(layer_keys[layer], false, now);
}

/* 레이어가 짧은 사이에 여러 번 바뀌면 마지막 상태만 한 번 보낸다 */
static K_WORK_DELAYABLE_DEFINE(send_work, send_layer);

static int layer_indicator_listener(const zmk_event_t *eh) {
    k_work_reschedule(&send_work, K_MSEC(30));
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_indicator, layer_indicator_listener);
ZMK_SUBSCRIPTION(layer_indicator, zmk_layer_state_changed);
