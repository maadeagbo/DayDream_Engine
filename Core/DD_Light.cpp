#include "DD_Light.h"

float LightSpace::CalculateLightVolumeRadius(const DD_Light* lght) {
  float LVR = 0.0f;
  float constant = 1.0;
  float lightMax =
      std::fmaxf(std::fmaxf(lght->m_color.r, lght->m_color.g), lght->m_color.b);
  LVR = (-lght->m_linear +
         std::sqrt(lght->m_linear * lght->m_linear -
                   4.f * lght->m_quadratic *
                       (constant - lightMax * (256.0f / 5.0f)))) /
        (2.f * lght->m_quadratic);
  return LVR;
}

void LightSpace::PrintInfo(const DD_Light& lght) {
  printf("\nLight ID: %s\n", lght.m_ID.c_str());

  if (lght.m_type == LightType::DIRECTION_L) {
    printf("\tLight type: Directional\n");
  } else if (lght.m_type == LightType::POINT_L) {
    printf("\tLight type: Point\n");
  } else {
    printf("\tLight type: Spot\n");
  }

  printf("\tPosition: %.3f, %.3f, %.3f\n", lght._position.x, lght._position.y,
         lght._position.z);
  printf("\tDirection: %.3f, %.3f, %.3f\n", lght.m_direction.x,
         lght.m_direction.y, lght.m_direction.z);
  printf("\tColor: %.3f, %.3f, %.3f\n", lght.m_color.r, lght.m_color.g,
         lght.m_color.b);
  printf("\tLinear fall-off: %f\n", lght.m_linear);
  printf("\tQuadratic fall-off: %f\n", lght.m_quadratic);
}
