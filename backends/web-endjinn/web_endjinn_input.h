#ifndef WEB_ENDJINN_INPUT_H
#define WEB_ENDJINN_INPUT_H

#include <cstddef>

namespace web_endjinn {

void gamepads_sample();
int gamepad_index_for_ordinal(std::size_t ordinal);
int gamepad_trigger(int gamepad_index, bool right_trigger);

}  // namespace web_endjinn

#endif  // WEB_ENDJINN_INPUT_H
