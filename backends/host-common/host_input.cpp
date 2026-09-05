#include <kos.h>

#include "host_audio.h"

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef ENJ_TARGET_WEB_ENDJINN
#include "web_endjinn_input.h"
#endif

namespace {

std::array<SDL_GameController *, MAPLE_PORT_COUNT> g_game_controllers{};
std::array<maple_device_t, MAPLE_PORT_COUNT> g_devices = {{
    {0, 0, true, {MAPLE_FUNC_CONTROLLER}},
    {1, 0, false, {MAPLE_FUNC_CONTROLLER}},
    {2, 0, false, {MAPLE_FUNC_CONTROLLER}},
    {3, 0, false, {MAPLE_FUNC_CONTROLLER}},
}};
std::array<cont_state_t, MAPLE_PORT_COUNT> g_controller_states{};
#ifdef ENJ_TARGET_WEB_ENDJINN
std::array<int, MAPLE_PORT_COUNT> g_web_gamepad_indices = {{-1, -1, -1, -1}};
#endif
struct MapleCallback {
    uint32_t function;
    maple_user_callback_t callback;
    void *user_data;
};
std::vector<MapleCallback> g_maple_attach_callbacks;
std::vector<MapleCallback> g_maple_detach_callbacks;
bool g_controllers_initialized = false;
bool g_quit_requested = false;

int open_controller_port(SDL_JoystickID instance)
{
    for (size_t port = 0; port < g_game_controllers.size(); port++) {
        SDL_GameController *controller = g_game_controllers[port];
        if (controller != nullptr &&
            SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) == instance) {
            return static_cast<int>(port);
        }
    }
    return -1;
}

bool controller_has_rumble(size_t port, SDL_GameController *controller)
{
    if (controller == nullptr) {
        return false;
    }
#ifdef ENJ_TARGET_WEB_ENDJINN
    return web_endjinn::gamepad_has_rumble(g_web_gamepad_indices[port]);
#elif SDL_VERSION_ATLEAST(2, 0, 18)
    (void)port;
    return SDL_GameControllerHasRumble(controller) == SDL_TRUE;
#else
    (void)port;
    return false;
#endif
}

void stop_controller_rumble(size_t port, SDL_GameController *controller)
{
    if (controller == nullptr) {
        return;
    }
#ifdef ENJ_TARGET_WEB_ENDJINN
    (void)web_endjinn::gamepad_rumble(
        g_web_gamepad_indices[port], 0.0, 0.0, 0);
#elif SDL_VERSION_ATLEAST(2, 0, 9)
    (void)port;
    (void)SDL_GameControllerRumble(controller, 0u, 0u, 0u);
#else
    (void)port;
#endif
}

void close_game_controllers()
{
    for (size_t port = 0; port < g_game_controllers.size(); port++) {
        SDL_GameController *&controller = g_game_controllers[port];
        if (controller != nullptr) {
            stop_controller_rumble(port, controller);
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
        g_devices[port].valid = port == 0;
        g_devices[port].info.functions = MAPLE_FUNC_CONTROLLER;
    }
#ifdef ENJ_TARGET_WEB_ENDJINN
    g_web_gamepad_indices.fill(-1);
#endif
}

void notify_maple_callbacks(
    const std::array<bool, MAPLE_PORT_COUNT> &previous_valid,
    const std::array<uint32_t, MAPLE_PORT_COUNT> &previous_functions)
{
    for (size_t port = 0; port < g_devices.size(); port++) {
        const maple_device_t &device = g_devices[port];
        for (const MapleCallback &entry : g_maple_detach_callbacks) {
            const bool was_present =
                previous_valid[port] &&
                (previous_functions[port] & entry.function) != 0u;
            const bool is_present =
                device.valid && (device.info.functions & entry.function) != 0u;
            if (was_present && !is_present && entry.callback != nullptr) {
                entry.callback(&g_devices[port], entry.user_data);
            }
        }
        for (const MapleCallback &entry : g_maple_attach_callbacks) {
            const bool was_present =
                previous_valid[port] &&
                (previous_functions[port] & entry.function) != 0u;
            const bool is_present =
                device.valid && (device.info.functions & entry.function) != 0u;
            if (!was_present && is_present && entry.callback != nullptr) {
                entry.callback(&g_devices[port], entry.user_data);
            }
        }
    }
}

void refresh_game_controllers()
{
    std::array<bool, MAPLE_PORT_COUNT> previous_valid{};
    std::array<uint32_t, MAPLE_PORT_COUNT> previous_functions{};
    for (size_t port = 0; port < g_devices.size(); port++) {
        previous_valid[port] = g_devices[port].valid;
        previous_functions[port] = g_devices[port].info.functions;
    }
#ifdef ENJ_TARGET_WEB_ENDJINN
    web_endjinn::gamepads_sample();
#endif
    for (size_t port = 0; port < g_game_controllers.size(); port++) {
        SDL_GameController *&controller = g_game_controllers[port];
        if (controller != nullptr && !SDL_GameControllerGetAttached(controller)) {
            stop_controller_rumble(port, controller);
            SDL_GameControllerClose(controller);
            controller = nullptr;
#ifdef ENJ_TARGET_WEB_ENDJINN
            g_web_gamepad_indices[port] = -1;
#endif
        }
    }
#ifdef ENJ_TARGET_WEB_ENDJINN
    size_t controller_ordinal = 0u;
#endif
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i)) {
            continue;
        }
#ifdef ENJ_TARGET_WEB_ENDJINN
        const int web_gamepad_index =
            web_endjinn::gamepad_index_for_ordinal(controller_ordinal);
        controller_ordinal++;
#endif
        const int open_port =
            open_controller_port(SDL_JoystickGetDeviceInstanceID(i));
        if (open_port >= 0) {
#ifdef ENJ_TARGET_WEB_ENDJINN
            if (web_gamepad_index >= 0) {
                g_web_gamepad_indices[static_cast<size_t>(open_port)] =
                    web_gamepad_index;
            }
#endif
            continue;
        }
        const auto slot = std::find(g_game_controllers.begin(), g_game_controllers.end(), nullptr);
        if (slot == g_game_controllers.end()) {
            break;
        }
        *slot = SDL_GameControllerOpen(i);
        if (*slot != nullptr) {
            const size_t port = static_cast<size_t>(slot - g_game_controllers.begin());
#ifdef ENJ_TARGET_WEB_ENDJINN
            g_web_gamepad_indices[port] = web_gamepad_index;
#endif
            std::fprintf(
                stderr,
                "host-enDjinn: controller connected on port %zu: %s\n", port,
                SDL_GameControllerName(*slot));
        }
    }
    for (size_t port = 0; port < g_devices.size(); port++) {
        SDL_GameController *controller = g_game_controllers[port];
        g_devices[port].valid = port == 0 || controller != nullptr;
        g_devices[port].info.functions = MAPLE_FUNC_CONTROLLER;
        if (controller_has_rumble(port, controller)) {
            g_devices[port].info.functions |= MAPLE_FUNC_PURUPURU;
        }
    }
    notify_maple_callbacks(previous_valid, previous_functions);
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

Sint16 controller_axis(SDL_GameController *controller, SDL_GameControllerAxis axis)
{
    return controller != nullptr ? SDL_GameControllerGetAxis(controller, axis)
        : 0;
}

uint8_t controller_trigger_to_u8(SDL_GameController *controller,
                                 SDL_GameControllerAxis axis, size_t port,
                                 bool right_trigger)
{
#ifdef ENJ_TARGET_WEB_ENDJINN
    const int web_value =
        web_endjinn::gamepad_trigger(g_web_gamepad_indices[port],
                                    right_trigger);
    if (web_value >= 0) {
        return static_cast<uint8_t>(web_value);
    }
#else
    (void)port;
    (void)right_trigger;
#endif
    return trigger_to_u8(controller_axis(controller, axis));
}

bool controller_button(SDL_GameController *controller, SDL_GameControllerButton button)
{
    return controller != nullptr && SDL_GameControllerGetButton(controller, button) != 0u;
}

}  // namespace

extern "C" {

void enj_host_input_request_quit(void)
{
    g_quit_requested = true;
}

void enj_host_input_shutdown(void)
{
    enj_host::audio_shutdown();
    close_game_controllers();
    g_maple_attach_callbacks.clear();
    g_maple_detach_callbacks.clear();
    g_controllers_initialized = false;
    g_quit_requested = false;
    SDL_Quit();
}

int enj_host_controllers_init(void)
{
    if (g_controllers_initialized) {
        return 0;
    }
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(
            stderr,
            "host-enDjinn: SDL controller initialization failed: %s\n",
            SDL_GetError());
        return -1;
    }
    SDL_GameControllerEventState(SDL_ENABLE);
    refresh_game_controllers();
    g_controllers_initialized = true;
    return 0;
}

void enj_host_controllers_shutdown(void)
{
    close_game_controllers();
}

size_t enj_host_controllers_poll(
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
    refresh_game_controllers();
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);
    if (keys == nullptr) {
        return 0u;
    }

    const size_t count = std::min(capacity, g_devices.size());
    for (size_t port = 0; port < count; port++) {
        SDL_GameController *controller = g_game_controllers[port];
        cont_state_t &state = states[port];
        const bool keyboard = port == 0;
        connected[port] = keyboard || controller != nullptr;
        state.rtrig = keyboard && keys[SDL_SCANCODE_V]
            ? 255u : controller_trigger_to_u8(
                controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, port, true);
        state.ltrig = keyboard && keys[SDL_SCANCODE_F]
            ? 255u : controller_trigger_to_u8(
                controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT, port, false);
        state.a = (keyboard && keys[SDL_SCANCODE_X]) || controller_button(controller, SDL_CONTROLLER_BUTTON_A) || (!keyboard && state.rtrig > 16);
        state.b = (keyboard && keys[SDL_SCANCODE_C]) || controller_button(controller, SDL_CONTROLLER_BUTTON_B) || (!keyboard && state.ltrig > 16);
        state.x = (keyboard && keys[SDL_SCANCODE_S]) || controller_button(controller, SDL_CONTROLLER_BUTTON_X);
        state.y = (keyboard && keys[SDL_SCANCODE_D]) || controller_button(controller, SDL_CONTROLLER_BUTTON_Y);
        state.start = (keyboard && keys[SDL_SCANCODE_RETURN]) || controller_button(controller, SDL_CONTROLLER_BUTTON_START);
        state.dpad_up = (keyboard && keys[SDL_SCANCODE_UP]) || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
        state.dpad_down = (keyboard && keys[SDL_SCANCODE_DOWN]) || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        state.dpad_left = (keyboard && keys[SDL_SCANCODE_LEFT]) || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        state.dpad_right = (keyboard && keys[SDL_SCANCODE_RIGHT]) || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        state.joyx = keyboard && keys[SDL_SCANCODE_J] ? -127 : (keyboard && keys[SDL_SCANCODE_L] ? 127 : axis_to_i8(controller_axis(controller, SDL_CONTROLLER_AXIS_LEFTX)));
        state.joyy = keyboard && keys[SDL_SCANCODE_I] ? -127 : (keyboard && keys[SDL_SCANCODE_K] ? 127 : axis_to_i8(controller_axis(controller, SDL_CONTROLLER_AXIS_LEFTY)));
    }
    if (g_quit_requested) {
        states[0].a = states[0].b = states[0].x = states[0].y = states[0].start = 1u;
    }
    return count;
}

maple_device_t *maple_enum_type(int index, uint32_t function)
{
    (void)enj_host_controllers_init();
    if (index < 0) {
        return nullptr;
    }
    int match = 0;
    for (maple_device_t &device : g_devices) {
        if (!device.valid || (device.info.functions & function) == 0u) {
            continue;
        }
        if (match == index) {
            return &device;
        }
        match++;
    }
    return nullptr;
}

maple_device_t *maple_enum_dev(int port, int unit)
{
    return port >= 0 && port < MAPLE_PORT_COUNT && unit == 0 && g_devices[port].valid
        ? &g_devices[port] : nullptr;
}

void *maple_dev_status(maple_device_t *device)
{
    if (device == nullptr || device->port < 0 || device->port >= MAPLE_PORT_COUNT) {
        return nullptr;
    }
    uint8_t connected[MAPLE_PORT_COUNT]{};
    (void)enj_host_controllers_poll(g_controller_states.data(), connected,
                                      MAPLE_PORT_COUNT);
    return connected[device->port] != 0u ? &g_controller_states[device->port] : nullptr;
}

void maple_attach_callback(uint32_t function, maple_user_callback_t callback,
                           void *user_data)
{
    if (callback == nullptr) {
        return;
    }
    const auto duplicate = std::find_if(
        g_maple_attach_callbacks.begin(), g_maple_attach_callbacks.end(),
        [=](const MapleCallback &entry) {
            return entry.function == function && entry.callback == callback &&
                entry.user_data == user_data;
        });
    if (duplicate == g_maple_attach_callbacks.end()) {
        g_maple_attach_callbacks.push_back({function, callback, user_data});
    }
}

void maple_detach_callback(uint32_t function, maple_user_callback_t callback,
                           void *user_data)
{
    if (callback == nullptr) {
        return;
    }
    const auto duplicate = std::find_if(
        g_maple_detach_callbacks.begin(), g_maple_detach_callbacks.end(),
        [=](const MapleCallback &entry) {
            return entry.function == function && entry.callback == callback &&
                entry.user_data == user_data;
        });
    if (duplicate == g_maple_detach_callbacks.end()) {
        g_maple_detach_callbacks.push_back({function, callback, user_data});
    }
}

void purupuru_rumble_raw(maple_device_t *device, uint32_t raw)
{
    if (device == nullptr || device->port < 0 ||
        device->port >= MAPLE_PORT_COUNT) {
        return;
    }
    const size_t port = static_cast<size_t>(device->port);
    SDL_GameController *controller = g_game_controllers[port];
    if (controller == nullptr) {
        return;
    }

    const uint32_t motor = (raw >> 4u) & 0x0fu;
    const uint32_t backward_power = (raw >> 8u) & 0x07u;
    const uint32_t forward_power = (raw >> 12u) & 0x07u;
    const uint32_t power = std::max(backward_power, forward_power);
    if (motor == 0u || power == 0u) {
        stop_controller_rumble(port, controller);
        return;
    }

    const uint32_t frequency = (raw >> 16u) & 0xffu;
    const uint32_t inclination = (raw >> 24u) & 0xffu;
    const bool continuous = (raw & 1u) != 0u;
    const double magnitude = static_cast<double>(power) / 7.0;
    const double high_mix = std::clamp(
        (static_cast<double>(frequency) - 4.0) / 55.0, 0.0, 1.0);
    const double low_frequency = magnitude * (1.0 - high_mix);
    const double high_frequency = magnitude * high_mix;
    const uint32_t duration_ms = continuous
        ? 60000u
        : inclination != 0u
            ? std::clamp(inclination * 8u, 45u, 2000u)
            : 120u;

#ifdef ENJ_TARGET_WEB_ENDJINN
    (void)web_endjinn::gamepad_rumble(
        g_web_gamepad_indices[port], low_frequency, high_frequency,
        static_cast<int>(duration_ms));
#elif SDL_VERSION_ATLEAST(2, 0, 9)
    const Uint16 low = static_cast<Uint16>(low_frequency * 65535.0 + 0.5);
    const Uint16 high = static_cast<Uint16>(high_frequency * 65535.0 + 0.5);
    (void)SDL_GameControllerRumble(controller, low, high, duration_ms);
#else
    (void)low_frequency;
    (void)high_frequency;
    (void)duration_ms;
#endif
}
}  // extern "C"
