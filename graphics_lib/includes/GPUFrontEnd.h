#include <inttypes.h>

struct DD_GPUData;

namespace DD_GPUFrontEnd {
// Wipe back-buffer screen
void clear_screen(const float r = 0.f, const float g = 0.f, const float b = 0.f, const float a = 1.f);
// initialize Graphics API library
bool load_api_library(const bool display_info = true);
}