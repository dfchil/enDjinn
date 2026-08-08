#include "web_endjinn_input.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <cstring>

namespace web_endjinn {

namespace {

bool g_sample_valid = false;

EM_JS(int, web_endjinn_gamepad_has_rumble_js, (int gamepad_index), {
    const getGamepads = navigator.getGamepads || navigator.webkitGetGamepads;
    if (!getGamepads) {
        return 0;
    }
    const gamepads = getGamepads.call(navigator);
    const gamepad = gamepads && gamepads[gamepad_index];
    return gamepad &&
        (gamepad.vibrationActuator ||
         (gamepad.hapticActuators && gamepad.hapticActuators.length > 0))
        ? 1 : 0;
});

EM_JS(int, web_endjinn_gamepad_rumble_js,
      (int gamepad_index, double low_frequency, double high_frequency,
       int duration_ms), {
    const getGamepads = navigator.getGamepads || navigator.webkitGetGamepads;
    if (!getGamepads) {
        return 0;
    }
    const gamepads = getGamepads.call(navigator);
    const gamepad = gamepads && gamepads[gamepad_index];
    const actuator = gamepad &&
        (gamepad.vibrationActuator ||
         (gamepad.hapticActuators && gamepad.hapticActuators[0]));
    if (!actuator) {
        return 0;
    }

    const ignoreRejection = result => {
        if (result && typeof result.catch === "function") {
            result.catch(() => {});
        }
    };
    if (duration_ms <= 0) {
        if (typeof actuator.reset === "function") {
            ignoreRejection(actuator.reset());
        } else if (typeof actuator.playEffect === "function") {
            ignoreRejection(actuator.playEffect("dual-rumble", {
                duration: 0,
                strongMagnitude: 0,
                weakMagnitude: 0
            }));
        } else if (typeof actuator.pulse === "function") {
            ignoreRejection(actuator.pulse(0, 0));
        }
        return 1;
    }

    low_frequency = Math.max(0, Math.min(1, low_frequency));
    high_frequency = Math.max(0, Math.min(1, high_frequency));
    if (typeof actuator.playEffect === "function") {
        ignoreRejection(actuator.playEffect("dual-rumble", {
            duration: duration_ms,
            startDelay: 0,
            strongMagnitude: low_frequency,
            weakMagnitude: high_frequency
        }));
        return 1;
    }
    if (typeof actuator.pulse === "function") {
        ignoreRejection(actuator.pulse(
            Math.max(low_frequency, high_frequency), duration_ms));
        return 1;
    }
    return 0;
});

int trigger_to_u8(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    return static_cast<int>(value * 255.0 + 0.5);
}

}  // namespace

void gamepads_sample()
{
    g_sample_valid =
        emscripten_sample_gamepad_data() == EMSCRIPTEN_RESULT_SUCCESS;
}

int gamepad_index_for_ordinal(std::size_t ordinal)
{
    if (!g_sample_valid) {
        return -1;
    }

    const int gamepad_count = emscripten_get_num_gamepads();
    for (int index = 0; index < gamepad_count; index++) {
        EmscriptenGamepadEvent state{};
        if (emscripten_get_gamepad_status(index, &state) !=
            EMSCRIPTEN_RESULT_SUCCESS) {
            continue;
        }
        if (ordinal == 0u) {
            return index;
        }
        ordinal--;
    }
    return -1;
}

int gamepad_trigger(int gamepad_index, bool right_trigger)
{
    if (!g_sample_valid || gamepad_index < 0) {
        return -1;
    }

    EmscriptenGamepadEvent state{};
    if (emscripten_get_gamepad_status(gamepad_index, &state) !=
            EMSCRIPTEN_RESULT_SUCCESS ||
        std::strcmp(state.mapping, "standard") != 0) {
        return -1;
    }

    // Firefox exposes standard-gamepad triggers as axes 4 and 5. Chrome
    // exposes the same controls as analog-valued buttons 6 and 7.
    const int trigger = right_trigger ? 1 : 0;
    if (state.numAxes >= 6) {
        return trigger_to_u8((state.axis[4 + trigger] + 1.0) * 0.5);
    }
    if (state.numButtons >= 8) {
        return trigger_to_u8(state.analogButton[6 + trigger]);
    }
    return -1;
}

bool gamepad_has_rumble(int gamepad_index)
{
    return gamepad_index >= 0 &&
        web_endjinn_gamepad_has_rumble_js(gamepad_index) != 0;
}

bool gamepad_rumble(int gamepad_index, double low_frequency,
                    double high_frequency, int duration_ms)
{
    return gamepad_index >= 0 &&
        web_endjinn_gamepad_rumble_js(
            gamepad_index, low_frequency, high_frequency, duration_ms) != 0;
}

}  // namespace web_endjinn
