#include <kos.h>

#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {

SDL_GameController *g_game_controller = nullptr;
maple_device_t g_keyboard_device{
    0,
    0,
    true,
    {MAPLE_FUNC_CONTROLLER},
};
cont_state_t g_controller_state{};
bool g_controllers_initialized = false;
bool g_quit_requested = false;

void refresh_game_controller()
{
    if (g_game_controller != nullptr &&
        !SDL_GameControllerGetAttached(g_game_controller)) {
        SDL_GameControllerClose(g_game_controller);
        g_game_controller = nullptr;
    }
    if (g_game_controller != nullptr) {
        return;
    }
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i)) {
            continue;
        }
        g_game_controller = SDL_GameControllerOpen(i);
        if (g_game_controller != nullptr) {
            std::fprintf(
                stderr,
                "pc-enDjinn: controller connected: %s\n",
                SDL_GameControllerName(g_game_controller));
            return;
        }
    }
}

int8_t axis_to_i8(Sint16 axis)
{
    constexpr int deadzone = 6000;
    if (axis > -deadzone && axis < deadzone) {
        return 0;
    }
    return static_cast<int8_t>(std::clamp<int>(axis / 256, -127, 127));
}

uint8_t trigger_to_u8(Sint16 axis)
{
    return axis > 0
        ? static_cast<uint8_t>(std::min<int>(axis / 128, 255))
        : 0u;
}

bool controller_button(SDL_GameControllerButton button)
{
    return g_game_controller != nullptr &&
        SDL_GameControllerGetButton(g_game_controller, button) != 0u;
}

Sint16 controller_axis(SDL_GameControllerAxis axis)
{
    return g_game_controller != nullptr
        ? SDL_GameControllerGetAxis(g_game_controller, axis)
        : 0;
}

}  // namespace

extern "C" {

void pc_endjinn_input_request_quit(void)
{
    g_quit_requested = true;
}

void pc_endjinn_input_shutdown(void)
{
    if (g_game_controller != nullptr) {
        SDL_GameControllerClose(g_game_controller);
        g_game_controller = nullptr;
    }
    g_controllers_initialized = false;
    g_quit_requested = false;
    SDL_Quit();
}

int pc_endjinn_controllers_init(void)
{
    if (g_controllers_initialized) {
        return 0;
    }
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(
            stderr,
            "pc-enDjinn: SDL controller initialization failed: %s\n",
            SDL_GetError());
        return -1;
    }
    SDL_GameControllerEventState(SDL_ENABLE);
    refresh_game_controller();
    g_controllers_initialized = true;
    return 0;
}

void pc_endjinn_controllers_shutdown(void)
{
    if (g_game_controller != nullptr) {
        SDL_GameControllerClose(g_game_controller);
        g_game_controller = nullptr;
    }
}

size_t pc_endjinn_controllers_poll(
    cont_state_t *states,
    uint8_t *connected,
    size_t capacity)
{
    if (states == nullptr || connected == nullptr || capacity == 0u) {
        return 0u;
    }
    std::memset(states, 0, capacity * sizeof(*states));
    std::memset(connected, 0, capacity * sizeof(*connected));

    SDL_PumpEvents();
    refresh_game_controller();
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);
    if (keys == nullptr) {
        return 0u;
    }

    cont_state_t &state = states[0];
    connected[0] = 1u;
    state.rtrig = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W]
        ? 255u
        : trigger_to_u8(controller_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    state.ltrig = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_S]
        ? 255u
        : trigger_to_u8(controller_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    state.a = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W] ||
        controller_button(SDL_CONTROLLER_BUTTON_A) || state.rtrig > 16u;
    state.b = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_S] ||
        controller_button(SDL_CONTROLLER_BUTTON_B) || state.ltrig > 16u;
    state.x = keys[SDL_SCANCODE_X] || keys[SDL_SCANCODE_R] ||
        controller_button(SDL_CONTROLLER_BUTTON_X);
    state.y = keys[SDL_SCANCODE_Y] || keys[SDL_SCANCODE_B] ||
        keys[SDL_SCANCODE_M] || keys[SDL_SCANCODE_E] ||
        controller_button(SDL_CONTROLLER_BUTTON_Y);
    state.start = keys[SDL_SCANCODE_RETURN] ||
        controller_button(SDL_CONTROLLER_BUTTON_START);
    state.dpad_up = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_F4] ||
        controller_button(SDL_CONTROLLER_BUTTON_DPAD_UP);
    state.dpad_down = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_F1] ||
        controller_button(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    state.dpad_left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_F2] ||
        controller_button(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    state.dpad_right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_F3] ||
        controller_button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    state.joyx = keys[SDL_SCANCODE_A] ? -127 :
        (keys[SDL_SCANCODE_D] ? 127 :
            axis_to_i8(controller_axis(SDL_CONTROLLER_AXIS_LEFTX)));
    state.joyy = keys[SDL_SCANCODE_W] ? -127 :
        (keys[SDL_SCANCODE_S] ? 127 :
            axis_to_i8(controller_axis(SDL_CONTROLLER_AXIS_LEFTY)));
    if (g_quit_requested) {
        state.a = 1u;
        state.b = 1u;
        state.x = 1u;
        state.y = 1u;
        state.start = 1u;
    }
    return 1u;
}

maple_device_t *maple_enum_type(int index, uint32_t function)
{
    (void)pc_endjinn_controllers_init();
    return index == 0 && (g_keyboard_device.info.functions & function) != 0u
        ? &g_keyboard_device
        : nullptr;
}

maple_device_t *maple_enum_dev(int port, int unit)
{
    return port == 0 && unit == 0 ? &g_keyboard_device : nullptr;
}

void *maple_dev_status(maple_device_t *device)
{
    if (device != &g_keyboard_device) {
        return nullptr;
    }
    uint8_t connected = 0u;
    (void)pc_endjinn_controllers_poll(&g_controller_state, &connected, 1u);
    return connected != 0u ? &g_controller_state : nullptr;
}

void maple_attach_callback(uint32_t, maple_user_callback_t, void *)
{
}

void maple_detach_callback(uint32_t, maple_user_callback_t, void *)
{
}

void snd_init(void) {}

sfxhnd_t snd_sfx_load_raw_buf(void *, size_t, uint32_t, uint8_t, uint8_t)
{
    return SFXHND_INVALID;
}

void snd_sfx_unload(sfxhnd_t) {}

int snd_sfx_play(sfxhnd_t, uint8_t, uint8_t)
{
    return -1;
}

void purupuru_rumble_raw(maple_device_t *, uint32_t) {}
void gdb_init(void) {}
void perf_monitor_init(int, int) {}
void perf_monitor_print(FILE *) {}
void arch_set_exit_path(int) {}
void vmufb_paint_area(vmufb_t *, unsigned int, unsigned int, unsigned int,
                      unsigned int, const uint8_t *) {}
void vmufb_clear(vmufb_t *fb)
{
    if (fb != nullptr) {
        std::memset(fb, 0, sizeof(*fb));
    }
}
void vmufb_present(const vmufb_t *, maple_device_t *) {}
void vmufb_print_string_into(vmufb_t *, const vmufb_font_t *, unsigned int,
                             unsigned int, unsigned int, unsigned int,
                             unsigned int, const char *) {}

}  // extern "C"
