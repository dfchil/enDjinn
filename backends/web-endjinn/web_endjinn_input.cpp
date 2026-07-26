#include "web_endjinn_input.h"

#include <emscripten/html5.h>

#include <algorithm>
#include <cstring>

namespace web_endjinn {

namespace {

bool g_sample_valid = false;

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

}  // namespace web_endjinn
