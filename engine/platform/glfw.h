#ifdef FU_USE_GLFW
#pragma once
#include "core/main.h"

#include "renderer/misc.h"

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

typedef enum _EGlfwKey {
    E_GLFW_KEY_NONE = 0,
    E_GLFW_KEY_SPACE = 32,
    E_GLFW_KEY_APOSTROPHE = 39, /* ' */
    E_GLFW_KEY_COMMA = 44, /* , */
    E_GLFW_KEY_MINUS, /* - */
    E_GLFW_KEY_PERIOD, /* . */
    E_GLFW_KEY_SLASH, /* / */
    E_GLFW_KEY_0,
    E_GLFW_KEY_1,
    E_GLFW_KEY_2,
    E_GLFW_KEY_3,
    E_GLFW_KEY_4,
    E_GLFW_KEY_5,
    E_GLFW_KEY_6,
    E_GLFW_KEY_7,
    E_GLFW_KEY_8,
    E_GLFW_KEY_9,
    E_GLFW_KEY_SEMICOLON = 59, // ;
    E_GLFW_KEY_EQUAL = 61, // =
    E_GLFW_KEY_A = 65,
    E_GLFW_KEY_B,
    E_GLFW_KEY_C,
    E_GLFW_KEY_D,
    E_GLFW_KEY_E,
    E_GLFW_KEY_F,
    E_GLFW_KEY_G,
    E_GLFW_KEY_H,
    E_GLFW_KEY_I,
    E_GLFW_KEY_J,
    E_GLFW_KEY_K,
    E_GLFW_KEY_L,
    E_GLFW_KEY_M,
    E_GLFW_KEY_N,
    E_GLFW_KEY_O,
    E_GLFW_KEY_P,
    E_GLFW_KEY_Q,
    E_GLFW_KEY_R,
    E_GLFW_KEY_S,
    E_GLFW_KEY_T,
    E_GLFW_KEY_U,
    E_GLFW_KEY_V,
    E_GLFW_KEY_W,
    E_GLFW_KEY_X,
    E_GLFW_KEY_Y,
    E_GLFW_KEY_Z,
    E_GLFW_KEY_LEFT_BRACKET, // [
    E_GLFW_KEY_BACKSLASH, /* \ */
    E_GLFW_KEY_RIGHT_BRACKET, // ]
    E_GLFW_KEY_GRAVE_ACCENT = 96, //  `
    /* Function keys */
    E_GLFW_KEY_ESCAPE = 256,
    E_GLFW_KEY_ENTER,
    E_GLFW_KEY_TAB,
    E_GLFW_KEY_BACKSPACE,
    E_GLFW_KEY_INSERT,
    E_GLFW_KEY_DELETE,
    E_GLFW_KEY_RIGHT,
    E_GLFW_KEY_LEFT,
    E_GLFW_KEY_DOWN,
    E_GLFW_KEY_UP,
    E_GLFW_KEY_PAGE_UP,
    E_GLFW_KEY_PAGE_DOWN,
    E_GLFW_KEY_HOME,
    E_GLFW_KEY_END,
    E_GLFW_KEY_CAPS_LOCK = 280,
    E_GLFW_KEY_SCROLL_LOCK,
    E_GLFW_KEY_NUM_LOCK,
    E_GLFW_KEY_PRINT_SCREEN,
    E_GLFW_KEY_PAUSE,
    E_GLFW_KEY_F1 = 290,
    E_GLFW_KEY_F2,
    E_GLFW_KEY_F3,
    E_GLFW_KEY_F4,
    E_GLFW_KEY_F5,
    E_GLFW_KEY_F6,
    E_GLFW_KEY_F7,
    E_GLFW_KEY_F8,
    E_GLFW_KEY_F9,
    E_GLFW_KEY_F10,
    E_GLFW_KEY_F11,
    E_GLFW_KEY_F12,
    E_GLFW_KEY_F13,
    E_GLFW_KEY_F14,
    E_GLFW_KEY_F15,
    E_GLFW_KEY_F16,
    E_GLFW_KEY_F17,
    E_GLFW_KEY_F18,
    E_GLFW_KEY_F19,
    E_GLFW_KEY_F20,
    E_GLFW_KEY_F21,
    E_GLFW_KEY_F22,
    E_GLFW_KEY_F23,
    E_GLFW_KEY_F24,
    E_GLFW_KEY_F25,
    E_GLFW_KEY_KP_0 = 320,
    E_GLFW_KEY_KP_1,
    E_GLFW_KEY_KP_2,
    E_GLFW_KEY_KP_3,
    E_GLFW_KEY_KP_4,
    E_GLFW_KEY_KP_5,
    E_GLFW_KEY_KP_6,
    E_GLFW_KEY_KP_7,
    E_GLFW_KEY_KP_8,
    E_GLFW_KEY_KP_9,
    E_GLFW_KEY_KP_DECIMAL,
    E_GLFW_KEY_KP_DIVIDE,
    E_GLFW_KEY_KP_MULTIPLY,
    E_GLFW_KEY_KP_SUBTRACT,
    E_GLFW_KEY_KP_ADD,
    E_GLFW_KEY_KP_ENTER,
    E_GLFW_KEY_KP_EQUAL,
    E_GLFW_KEY_LEFT_SHIFT = 340,
    E_GLFW_KEY_LEFT_CONTROL,
    E_GLFW_KEY_LEFT_ALT,
    E_GLFW_KEY_LEFT_SUPER,
    E_GLFW_KEY_RIGHT_SHIFT,
    E_GLFW_KEY_RIGHT_CONTROL,
    E_GLFW_KEY_RIGHT_ALT,
    E_GLFW_KEY_RIGHT_SUPER,
    E_GLFW_KEY_MENU,
    E_GLFW_KEY_CNT
} EGlfwKey;

typedef struct _FUKeyboardEvent {
    EGlfwKey key;
    int scancode;
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

bool fu_window_present(FUWindow* win);
#endif // FU_USE_GLFW