#pragma once

#include "GPUFrontEnd.h"
#include "ddIncludes.h"

enum class TexType : unsigned {
  ALBEDO = 0x1,
  SPEC = 0x2,
  NORMAL = 0x4,
  ROUGH = 0x8,
  METAL = 0x10,
  EMISSIVE = 0x20,
  AMBIENT = 0x40,
  CUBE = 0x80,
  NULL_T = 0x100
};
ENABLE_BITMASK_OPERATORS(TexType)

struct ddTex2D {
  /** \brief Engine identifier assigned at initialization */
  size_t id;
  /** \brief Image information on GPU and RAM */
  ImageInfo image_info;
};
