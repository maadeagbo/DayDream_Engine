// Enums for managing shader uniforms

enum class RE_Light : int {
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

enum class RE_GBuffer : int {
  enable_clip1_b = 0,
  MVP_m4x4 = 1,
  Norm_m4x4 = 2,
  multiplierMat_b = 3,
  tex_albedo_smp2d = 4,
  tex_specular_smp2d = 5,
  tex_normal_smp2d = 6,
  diffuse_v3 = 7,
  shininess_f = 8,
  albedoFlag_b = 9,
  specFlag_b = 10,
  normalFlag_b = 11,
  useDebug_b = 12
};

enum class RE_PostPr : int {
  MVP_m4x4 = 0,
  SampleShadow_b = 1,
  flip_y_coord_b = 2,
  flip_x_coord_b = 3,
  uv_rotate_b = 4,
  rotate_uv_mat_m4x4 = 5,
  ColorTex_smp2d = 6,
  ParticleTex_smp2d = 7,
  rgb2xyz_m3x3 = 8,
  xyz2rgb_m3x3 = 9,
  AveLum_f = 10,
  Exposure_f = 11,
  White_f = 12,
  DoToneMap_b = 13,
  SampleMap_b = 14,
  Blur_b = 15,
  GammaCorrect_b = 16,
  BlendParticle_b = 17,
  output2D_b = 18,
  direction_flag_v2 = 19
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
  img_input_img2D = 0,
  img_output_img2D = 1,
  computeLum_b = 2
};

enum class RE_Line : int {
  MVP_m4x4 = 0,
  color_v4 = 1
};

enum class RE_LightSt : int {
  MVP_m4x4 = 0
};

enum class RE_Shadow : int {
  MVP_m4x4 = 0,
  LightSpace_m4x4 = 1
};

enum class RE_ShadowSk : int {
  MVP_m4x4 = 0,
  LightSpace_m4x4 = 1
};

