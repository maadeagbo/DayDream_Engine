// Enums for managing shader uniforms

enum class shaderTest : unsigned {
  MVP_m4x4 = 0,
  DrawSky_b = 1,
  LightVolume_b = 2,
  PositionTex_smp2d = 3,
  ColorTex_smp2d = 4,
  NormalTex_smp2d = 5,
  DepthTex_smp2d = 6,
  skybox_smpCube = 7,
  Light_position_v3 = 8,
  Light_direction_v3 = 9,
  Light_color_v3 = 10,
  Light_type_i = 11,
  Light_cutoff_i_f = 12,
  Light_cutoff_o_f = 13,
  Light_spotExponent_f = 14,
  Light_linear_f = 15,
  Light_quadratic_f = 16,
  Light_attenuation_f = 17,
  LSM_m4x4 = 18,
  viewPos_v3 = 19,
  screenDimension_v2 = 20,
  ShadowMap_b = 21
};
