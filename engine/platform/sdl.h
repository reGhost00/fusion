#ifdef FU_USE_SDL
#pragma once
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

// custom
#include "core/main.h"

#include "renderer/misc.h"

typedef enum _EFUModKey {
    EFU_MOD_KEY_SHIFT_LEFT = KMOD_LSHIFT,
    EFU_MOD_KEY_SHIFT_RIGHT = KMOD_RSHIFT,
    EFU_MOD_KEY_SHIFT = KMOD_LSHIFT | KMOD_RSHIFT,
    EFU_MOD_KEY_CTRL_LEFT = KMOD_LCTRL,
    EFU_MOD_KEY_CTRL_RIGHT = KMOD_RCTRL,
    EFU_MOD_KEY_CTRL = KMOD_LCTRL | KMOD_RCTRL,
    EFU_MOD_KEY_ALT_LEFT = KMOD_LALT,
    EFU_MOD_KEY_ALT_RIGHT = KMOD_RALT,
    EFU_MOD_KEY_ALT = KMOD_LALT | KMOD_RALT,
    EFU_MOD_KEY_SUPER_LEFT = KMOD_LGUI,
    EFU_MOD_KEY_SUPER_RIGHT = KMOD_RGUI,
    EFU_MOD_KEY_SUPER = KMOD_LGUI | KMOD_RGUI,
    EFU_MOD_KEY_CAPS_LOCK = KMOD_CAPS,
    EFU_MOD_KEY_NUM_LOCK = KMOD_NUM
} EFUModKey;

typedef enum _EFUGamepadButton {
    EFU_GAMEPAD_BUTTON_A,
    EFU_GAMEPAD_BUTTON_B,
    EFU_GAMEPAD_BUTTON_X,
    EFU_GAMEPAD_BUTTON_Y,
    EFU_GAMEPAD_BUTTON_LEFT_BUMPER,
    EFU_GAMEPAD_BUTTON_RIGHT_BUMPER,
    EFU_GAMEPAD_BUTTON_BACK,
    EFU_GAMEPAD_BUTTON_START,
    EFU_GAMEPAD_BUTTON_GUIDE,
    EFU_GAMEPAD_BUTTON_LEFT_THUMB,
    EFU_GAMEPAD_BUTTON_RIGHT_THUMB,
    EFU_GAMEPAD_BUTTON_DPAD_UP,
    EFU_GAMEPAD_BUTTON_DPAD_RIGHT,
    EFU_GAMEPAD_BUTTON_DPAD_DOWN,
    EFU_GAMEPAD_BUTTON_DPAD_LEFT,
    EFU_GAMEPAD_BUTTON_CNT
} EFUGamepadButton;

typedef enum _EFUGamepadAxis {
    EFU_GAMEPAD_AXIS_LEFT_X,
    EFU_GAMEPAD_AXIS_LEFT_Y,
    EFU_GAMEPAD_AXIS_RIGHT_X,
    EFU_GAMEPAD_AXIS_RIGHT_Y,
    EFU_GAMEPAD_AXIS_LEFT_TRIGGER,
    EFU_GAMEPAD_AXIS_RIGHT_TRIGGER,
    EFU_GAMEPAD_AXIS_CNT
} EFUGamepadAxis;

typedef struct _FUKeyboardEvent {
    SDL_Keycode key;
    SDL_Scancode scancode;
    EFUModKey mods;
    const char* name;
} FUKeyboardEvent;

typedef enum _EFUMouseButton {
    EFU_MOUSE_BUTTON_LEFT,
    EFU_MOUSE_BUTTON_RIGHT,
    EFU_MOUSE_BUTTON_MIDDLE,
    EFU_MOUSE_BUTTON_4,
    EFU_MOUSE_BUTTON_5,
    EFU_MOUSE_BUTTON_6,
    EFU_MOUSE_BUTTON_7,
    EFU_MOUSE_BUTTON_8,
    EFU_MOUSE_BUTTON_CNT
} EFUMouseButton;

typedef struct _FUMouseEvent {
    FUOffset2 position;
    EFUMouseButton button;
    EFUModKey mods;
} FUMouseEvent;

typedef struct _FUScrollEvent {
    FUOffset2 offset;
    FUOffset2 delta;
} FUScrollEvent;

FU_DECLARE_TYPE(FUWindow, fu_window)
#define FU_TYPE_WINDOW (fu_window_get_type())

typedef struct _FUContext FUContext;

FUWindow* fu_window_new(FUApplication* app, const char* title, uint32_t width, uint32_t height, bool resizable);
bool fu_window_add_context(FUWindow* win, FUContext* ctx);

void fu_window_take_source(FUWindow* win, FUSource** source);

void fu_window_present(FUWindow* win, FUContext* ctx);

#endif