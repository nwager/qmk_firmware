/* Copyright 2021 Glorious, LLC <salman@pcgamingrace.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include "utils.h"

#define LRGB_VA_STEPS 10
#define LRGB_CONVERT_STEP(x, s) (((int)(x) * 255) / (s))

// Each layer gets a name for readability, which is then used in the keymap matrix below.
// The underscores don't mean anything - you can have a layer called STUFF or any other name.

enum layers {
    L_BASE,  // default, swap GESC and GRV for programming
    L_GAME,  // gaming layer, put back GESC since menus use ESC a lot
    L_FN,    // fn layer
    L_NUM_,  // use this for number of layers, not an actual layer
};

enum custom_keycodes {
    KC_LRGB_TOG = SAFE_RANGE,
    KC_LRGB_RST,
    KC_LRGB_VAI,
    KC_LRGB_VAD,
};

struct rgb_state {
    bool on;
    uint8_t h, s, v, spd;
    int mode;
};

static struct rgb_state PROGMEM LRGB_DEFAULT[L_NUM_] = {
    [L_BASE] = {
        .on   = true,
        .spd  = 14,
        .s    = 255,
        .v    = 7,
        .mode = RGB_MATRIX_CYCLE_ALL,
    },
    [L_GAME] = {
        .on   = true,
        .spd  = 48,
        .s    = 255,
        .v    = 7,
        .mode = RGB_MATRIX_CYCLE_OUT_IN,
    },
    [L_FN] = {
        .on   = true,
        .mode = RGB_MATRIX_NONE,
    }
};
static struct rgb_state lrgb_state[L_NUM_];

// Update matrix for the current state.
static void rgb_matrix_refresh(uint32_t state);
// Get highest non-NONE active layer.
static uint8_t get_highest_rgb_layer(uint32_t state);
// Reset RGB settings to default.
static void lrgb_reset(void);

#define KC_FN MO(L_FN)
#define FN_LED 63

#define CAPS_LED 30

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[L_BASE] = LAYOUT(
   KC_GRV,  KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,  KC_DEL,
  KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,   KC_PGUP,
  KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_ENT,            KC_PGDN,
  KC_LSFT,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,    KC_END,
  KC_LCTL,  KC_LGUI,  KC_LALT,                                KC_SPC,                                 KC_RALT,  KC_FN,    KC_LEFT,  KC_DOWN,  KC_RGHT),

[L_GAME] = LAYOUT(
  KC_GESC,  KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,  KC_DEL,
  KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,   KC_PGUP,
  KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_ENT,            KC_PGDN,
  KC_LSFT,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,    KC_END,
  KC_LCTL,  KC_LGUI,  KC_LALT,                                KC_SPC,                                 KC_RALT,  KC_FN,    KC_LEFT,  KC_DOWN,  KC_RGHT),

[L_FN] = LAYOUT(
  KC_GESC,    KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,   KC_F10,   KC_F11,   KC_F12,   _______,  KC_INS,
  _______,   KC_GRV,  _______,  DB_TOGG,  QK_BOOT,  _______,  _______,  KC_LRGB_RST,  KC_LRGB_TOG,  RGB_TOG,  KC_PSCR,  KC_SCRL,  KC_PAUS,   _______,  _______,
  _______,  _______,  _______,  _______,  _______, TG(L_GAME), _______,  _______,  _______,  _______,  _______,  _______,             _______,
  _______,  _______,  RGB_HUI,  RGB_HUD,  RGB_SPD,  RGB_SPI,  KC_MUTE,  KC_VOLU,  KC_VOLD,  KC_MPRV,  KC_MPLY,  KC_MNXT,  _______,  KC_LRGB_VAI,  KC_HOME,
  _______,  _______,  _______,                                RESET,                                  _______,  _______,  _______,  KC_LRGB_VAD,  _______),
};

void keyboard_post_init_user(void) {
    // initialize matrix state
    lrgb_reset();
    rgb_matrix_refresh(layer_state);	
}

static void lrgb_reset(void) {
    // Basically memcpy but too lazy to #include it
    for (uint8_t i = L_BASE; i < L_NUM_; i++)
        lrgb_state[i] = LRGB_DEFAULT[i];
}

uint32_t layer_state_set_user(uint32_t state) {
    rgb_matrix_refresh(state);
    return state;
}

void rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // light up L_FN key when active
    if (IS_LAYER_ON_STATE(layer_state, L_FN)) {
        RGB_MATRIX_INDICATOR_SET_COLOR(FN_LED, 255, 255, 255);
    }
    // light up caps lock when active
    if (host_keyboard_led_state().caps_lock) {
        RGB_MATRIX_INDICATOR_SET_COLOR(CAPS_LED, 255, 255, 255);
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        uint8_t l = get_highest_rgb_layer(layer_state);
        switch (keycode) {
            // rgb keycodes
            case KC_LRGB_TOG:
                lrgb_state[l].on = !lrgb_state[l].on;
                goto kc_lrgb_exit;
            case KC_LRGB_RST:
                lrgb_reset();
                goto kc_lrgb_exit;
            case KC_LRGB_VAI:
                lrgb_state[l].v = clamp(lrgb_state[l].v + 1, 0, LRGB_VA_STEPS);
                goto kc_lrgb_exit;
            case KC_LRGB_VAD:
                lrgb_state[l].v = clamp(lrgb_state[l].v - 1, 0, LRGB_VA_STEPS);
                goto kc_lrgb_exit;
            default:
                return true;
        }
    }
    return true;

kc_lrgb_exit:
    rgb_matrix_refresh(layer_state);
    return true;
}

static void rgb_matrix_refresh(uint32_t state) {
    uint8_t layer = get_highest_rgb_layer(state);
    struct rgb_state rgbs = lrgb_state[layer];
    if (rgbs.on) {
        rgb_matrix_sethsv(rgbs.h, rgbs.s, LRGB_CONVERT_STEP(rgbs.v, LRGB_VA_STEPS));
        rgb_matrix_set_speed(rgbs.spd);
        rgb_matrix_mode(rgbs.mode);
        return;
    }
    rgb_matrix_sethsv(0, 0, 0);
    rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
}

static uint8_t get_highest_rgb_layer(uint32_t state) {
    // Get highest non-NONE layer. Default layer is always non-NONE and on.
    for (uint8_t i = L_NUM_-1; i > L_BASE; i--) {
        if (IS_LAYER_ON_STATE(state, i) && lrgb_state[i].mode != RGB_MATRIX_NONE)
            return i;
    }
    return L_BASE;
}
