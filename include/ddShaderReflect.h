// Enums for managing shader uniforms

enum class RE_Light : int {
  ColorTex_smp2d = 0,
  DepthTex_smp2d = 1,
  DrawSky_b = 2,
  LSM_m4x4 = 3,
  Light_color_v3 = 4,
  Light_cutoff_i_f = 5,
  Light_cutoff_o_f = 6,
  Light_direction_v3 = 7,
  Light_linear_f = 8,
  Light_position_v3 = 9,
  Light_quadratic_f = 10,
  Light_spotExponent_f = 11,
  Light_type_i = 12,
  LightVolume_b = 13,
  MVP_m4x4 = 14,
  NormalTex_smp2d = 15,
  PositionTex_smp2d = 16,
  ShadowMap_b = 17,
  screenDimension_v2 = 18,
  skybox_smpCube = 19,
  viewPos_v3 = 20
};

enum class RE_GBuffer : int {
  MVP_m4x4 = 0,
  Norm_m4x4 = 1,
  albedoFlag_b = 2,
  diffuse_v3 = 3,
  multiplierMat_b = 4,
  normalFlag_b = 5,
  shininess_f = 6,
  specFlag_b = 7,
  tex_albedo_smp2d = 8,
  tex_normal_smp2d = 9,
  tex_specular_smp2d = 10,
  useDebug_b = 11
};

enum class RE_PostPr : int {
  AveLum_f = 0,
  BlendParticle_b = 1,
  Blur_b = 2,
  ColorTex_smp2d = 3,
  DoToneMap_b = 4,
  Exposure_f = 5,
  GammaCorrect_b = 6,
  MVP_m4x4 = 7,
  ParticleTex_smp2d = 8,
  SampleMap_b = 9,
  SampleShadow_b = 10,
  White_f = 11,
  direction_flag_v2 = 12,
  flip_x_coord_b = 13,
  flip_y_coord_b = 14,
  output2D_b = 15,
  rgb2xyz_m3x3 = 16,
  rotate_uv_mat_m4x4 = 17,
  uv_rotate_b = 18,
  xyz2rgb_m3x3 = 19
};

enum class RE_Text : int {
  projection_m4x4 = 0,
  text_smp2d = 1,
  textColor_v3 = 2
};

enum class RE_PingP : int {
  ColorTex_smp2d = 0
};

enum class RE_Lumin : int {
  computeLum_b = 0,
  img_input_img2D = 1,
  img_output_img2D = 2
};

enum class RE_Line : int {
  MVP_m4x4 = 0,
  color_v4 = 1
};

enum class RE_LightSt : int {
  MVP_m4x4 = 0
};

enum class RE_Shadow : int {
  LightSpace_m4x4 = 0,
  MVP_m4x4 = 1
};

enum class RE_ShadowSk : int {
  LightSpace_m4x4 = 0,
  MVP_m4x4 = 1
};

