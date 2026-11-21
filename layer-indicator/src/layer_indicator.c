/*
 * 레이어가 바뀌면 지금 가장 위에 켜진 레이어를 F16~F19 키로 컴퓨터에 알린다.
 *   레이어 0 → F16, 1 → F17, 2 → F18, 3 → F19
 * 맥의 Hammerspoon 스크립트가 이 키를 받아 표시하고, 다른 앱에는 전달하지 않는다.
 *
 * 전달이 빠지지 않도록:
 *   - 키를 누른 뒤 30ms 동안 누르고 있다가 뗀다 (누름과 뗌을 한 번에 보내지 않음)
 *   - 같은 신호를 300ms 뒤에 한 번 더 보낸다
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
static uint32_t held_key;
static int repeats_left;

static void release_key(struct k_work *work);
static void press_key(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(release_work, release_key);
static K_WORK_DELAYABLE_DEFINE(press_work, press_key);

static void press_key(struct k_work *work) {
    if (held_key) {
        /* 아직 떼지 않은 신호가 있으면 먼저 떼고 조금 뒤 다시 시도 */
        k_work_reschedule(&press_work, K_MSEC(40));
        return;
    }
    uint8_t layer = zmk_keymap_highest_layer_active();
    if (layer >= ARRAY_SIZE(layer_keys)) {
        return;
    }
    held_key = layer_keys[layer];
    raise_zmk_keycode_state_changed_from_encoded(held_key, true, k_uptime_get());
    k_work_reschedule(&release_work, K_MSEC(30));
}

static void release_key(struct k_work *work) {
    if (!held_key) {
        return;
    }
    raise_zmk_keycode_state_changed_from_encoded(held_key, false, k_uptime_get());
    held_key = 0;
    if (repeats_left > 0) {
        repeats_left--;
        k_work_reschedule(&press_work, K_MSEC(300));
    }
}

static int layer_indicator_listener(const zmk_event_t *eh) {
    repeats_left = 1;
    k_work_reschedule(&press_work, K_MSEC(30));
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_indicator, layer_indicator_listener);
ZMK_SUBSCRIPTION(layer_indicator, zmk_layer_state_changed);
