#ifdef FU_USE_SDL
#pragma once
#include "core/main.h"
#include "misc.h"
typedef enum _EFUKeyboardKey {
    EFU_KB_KEY_NONE = 0,
    EFU_KB_KEY_SPACE = 32,
    EFU_KB_KEY_APOSTROPHE = 39, /* ' */
    EFU_KB_KEY_COMMA = 44, /* , */
    EFU_KB_KEY_MINUS, /* - */
    EFU_KB_KEY_PERIOD, /* . */
    EFU_KB_KEY_SLASH, /* / */
    EFU_KB_KEY_0,
    EFU_KB_KEY_1,
    EFU_KB_KEY_2,
    EFU_KB_KEY_3,
    EFU_KB_KEY_4,
    EFU_KB_KEY_5,
    EFU_KB_KEY_6,
    EFU_KB_KEY_7,
    EFU_KB_KEY_8,
    EFU_KB_KEY_9,
    EFU_KB_KEY_SEMICOLON = 59, // ;
    EFU_KB_KEY_EQUAL = 61, // =
    EFU_KB_KEY_A = 65,
    EFU_KB_KEY_B,
    EFU_KB_KEY_C,
    EFU_KB_KEY_D,
    EFU_KB_KEY_E,
    EFU_KB_KEY_F,
    EFU_KB_KEY_G,
    EFU_KB_KEY_H,
    EFU_KB_KEY_I,
    EFU_KB_KEY_J,
    EFU_KB_KEY_K,
    EFU_KB_KEY_L,
    EFU_KB_KEY_M,
    EFU_KB_KEY_N,
    EFU_KB_KEY_O,
    EFU_KB_KEY_P,
    EFU_KB_KEY_Q,
    EFU_KB_KEY_R,
    EFU_KB_KEY_S,
    EFU_KB_KEY_T,
    EFU_KB_KEY_U,
    EFU_KB_KEY_V,
    EFU_KB_KEY_W,
    EFU_KB_KEY_X,
    EFU_KB_KEY_Y,
    EFU_KB_KEY_Z,
    EFU_KB_KEY_LEFT_BRACKET, // [
    EFU_KB_KEY_BACKSLASH, /* \ */
    EFU_KB_KEY_RIGHT_BRACKET, // ]
    EFU_KB_KEY_GRAVE_ACCENT = 96, //  `
    /* Function keys */
    EFU_KB_KEY_ESCAPE = 256,
    EFU_KB_KEY_ENTER,
    EFU_KB_KEY_TAB,
    EFU_KB_KEY_BACKSPACE,
    EFU_KB_KEY_INSERT,
    EFU_KB_KEY_DELETE,
    EFU_KB_KEY_RIGHT,
    EFU_KB_KEY_LEFT,
    EFU_KB_KEY_DOWN,
    EFU_KB_KEY_UP,
    EFU_KB_KEY_PAGE_UP,
    EFU_KB_KEY_PAGE_DOWN,
    EFU_KB_KEY_HOME,
    EFU_KB_KEY_END,
    EFU_KB_KEY_CAPS_LOCK = 280,
    EFU_KB_KEY_SCROLL_LOCK,
    EFU_KB_KEY_NUM_LOCK,
    EFU_KB_KEY_PRINT_SCREEN,
    EFU_KB_KEY_PAUSE,
    EFU_KB_KEY_F1 = 290,
    EFU_KB_KEY_F2,
    EFU_KB_KEY_F3,
    EFU_KB_KEY_F4,
    EFU_KB_KEY_F5,
    EFU_KB_KEY_F6,
    EFU_KB_KEY_F7,
    EFU_KB_KEY_F8,
    EFU_KB_KEY_F9,
    EFU_KB_KEY_F10,
    EFU_KB_KEY_F11,
    EFU_KB_KEY_F12,
    EFU_KB_KEY_F13,
    EFU_KB_KEY_F14,
    EFU_KB_KEY_F15,
    EFU_KB_KEY_F16,
    EFU_KB_KEY_F17,
    EFU_KB_KEY_F18,
    EFU_KB_KEY_F19,
    EFU_KB_KEY_F20,
    EFU_KB_KEY_F21,
    EFU_KB_KEY_F22,
    EFU_KB_KEY_F23,
    EFU_KB_KEY_F24,
    EFU_KB_KEY_F25,
    EFU_KB_KEY_KP_0 = 320,
    EFU_KB_KEY_KP_1,
    EFU_KB_KEY_KP_2,
    EFU_KB_KEY_KP_3,
    EFU_KB_KEY_KP_4,
    EFU_KB_KEY_KP_5,
    EFU_KB_KEY_KP_6,
    EFU_KB_KEY_KP_7,
    EFU_KB_KEY_KP_8,
    EFU_KB_KEY_KP_9,
    EFU_KB_KEY_KP_DECIMAL,
    EFU_KB_KEY_KP_DIVIDE,
    EFU_KB_KEY_KP_MULTIPLY,
    EFU_KB_KEY_KP_SUBTRACT,
    EFU_KB_KEY_KP_ADD,
    EFU_KB_KEY_KP_ENTER,
    EFU_KB_KEY_KP_EQUAL,
    EFU_KB_KEY_LEFT_SHIFT = 340,
    EFU_KB_KEY_LEFT_CONTROL,
    EFU_KB_KEY_LEFT_ALT,
    EFU_KB_KEY_LEFT_SUPER,
    EFU_KB_KEY_RIGHT_SHIFT,
    EFU_KB_KEY_RIGHT_CONTROL,
    EFU_KB_KEY_RIGHT_ALT,
    EFU_KB_KEY_RIGHT_SUPER,
    EFU_KB_KEY_MENU,
    EFU_KB_KEY_CNT
} EFUKeyboardKey;

typedef enum _EFUModKey {
    EFU_MOD_KEY_SHIFT = 0x0001,
    EFU_MOD_KEY_CTRL = 0x0002,
    EFU_MOD_KEY_ALT = 0x0004,
    EFU_MOD_KEY_SUPER = 0x0008,
    EFU_MOD_KEY_CAPS_LOCK = 0x0010,
    EFU_MOD_KEY_NUM_LOCK = 0x0020
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
    EFUKeyboardKey key;
    EFUModKey mods;
    const char* name;
    int scanCode;
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

uint64_t fu_window_get_type();
typedef struct _FUWindow FUWindow;
#define FU_TYPE_WINDOW (fu_window_get_type())

FUWindow* fu_window_new(FUApp* app, FUWindowConfig* cfg);
void fu_window_take_source(FUWindow* win, FUSource** source);

#endif