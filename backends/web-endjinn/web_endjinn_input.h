#ifndef WEB_ENDJINN_INPUT_H
#define WEB_ENDJINN_INPUT_H

#include <cstddef>

namespace web_endjinn {

void gamepads_sample();
int gamepad_index_for_ordinal(std::size_t ordinal);
int gamepad_trigger(int gamepad_index, bool right_trigger);
bool gamepad_has_rumble(int gamepad_index);
bool gamepad_rumble(int gamepad_index, double low_frequency,
                    double high_frequency, int duration_ms);

}  // namespace web_endjinn

#endif  // WEB_ENDJINN_INPUT_H
