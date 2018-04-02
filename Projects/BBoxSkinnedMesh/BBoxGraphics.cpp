#include "BBoxGraphics.h"
#include "ddSceneManager.h"

void BBoxGraphics::generate_grid() {
  // generate points (x axis, then y axis, then z axis)
  float curr_value = MAX_DIMEN_VAL / 2;
  const unsigned stride = NUM_GRID_AXIS * 3;
  for (unsigned i = 0; i < NUM_GRID_LINES; i++) {
    // x-axis
    grid_points[i * stride] = curr_value;
    grid_points[i * stride + 1] = 0;
    grid_points[i * stride + 2] = MAX_DIMEN_VAL / 2;

    // y-axis 1
    grid_points[i * stride + 3] = 0;
    grid_points[i * stride + 3 + 1] = MAX_DIMEN_VAL / 2;
    grid_points[i * stride + 3 + 2] = curr_value;

    // y-axis 2
    grid_points[i * stride + 9] = 0;
    grid_points[i * stride + 9 + 1] = curr_value;
    grid_points[i * stride + 9 + 2] = MAX_DIMEN_VAL / 2;

    // z-axis
    grid_points[i * stride + 6] = MAX_DIMEN_VAL / 2;
    grid_points[i * stride + 6 + 1] = 0;
    grid_points[i * stride + 6 + 2] = curr_value;

    // update curr_value
    curr_value -= 1.f;
  }

  curr_value = MAX_DIMEN_VAL / 2;
  const unsigned offset = NUM_GRID_LINES * NUM_GRID_AXIS * 3;
  for (unsigned i = 0; i < NUM_GRID_LINES; i++) {
    // x-axis
    grid_points[offset + i * stride] = curr_value;
    grid_points[offset + i * stride + 1] = 0;
    grid_points[offset + i * stride + 2] = -MAX_DIMEN_VAL / 2;

    // y-axis 1
    grid_points[offset + i * stride + 3] = 0;
    grid_points[offset + i * stride + 3 + 1] = -MAX_DIMEN_VAL / 2;
    grid_points[offset + i * stride + 3 + 2] = curr_value;

    // y-axis 2
    grid_points[offset + i * stride + 9] = 0;
    grid_points[offset + i * stride + 9 + 1] = curr_value;
    grid_points[offset + i * stride + 9 + 2] = -MAX_DIMEN_VAL / 2;

    // z-axis
    grid_points[offset + i * stride + 6] = -MAX_DIMEN_VAL / 2;
    grid_points[offset + i * stride + 6 + 1] = 0;
    grid_points[offset + i * stride + 6 + 2] = curr_value;

    // update curr_value
    curr_value -= 1.f;
  }

  // generate indices
  const unsigned offset_2 = NUM_GRID_LINES * NUM_GRID_AXIS;
  for (unsigned i = 0; i < (NUM_GRID_LINES * NUM_GRID_AXIS); i++) {
    grid_indices[i * 2] = i;
    grid_indices[i * 2 + 1] = i + offset_2;
  }
}
