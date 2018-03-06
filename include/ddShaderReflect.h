// Enums for managing shader uniforms

enum class RE_Light : int {
  ColorTex_smp2d = 0,
  Debug_b = 1,
  Debug_Color_v4 = 2,
  DepthTex_smp2d = 3,
  DrawSky_b = 4,
  LSM_m4x4 = 5,
  Light_color_v3 = 6,
  Light_cutoff_i_f = 7,
  Light_cutoff_o_f = 8,
  Light_direction_v3 = 9,
  Light_lumin_cutoff_f = 10,
  Light_position_v3 = 11,
  Light_type_i = 12,
  Model_m4x4 = 13,
  NormalTex_smp2d = 14,
  PositionTex_smp2d = 15,
  Proj_m4x4 = 16,
  ShadowMap_b = 17,
  View_m4x4 = 18,
  screenDimension_v2 = 19,
  skybox_smpCube = 20,
  viewPos_v3 = 21
};

enum class RE_GBuffer : int {
  Model_m4x4 = 0,
  Norm_m4x4 = 1,
  VP_m4x4 = 2,
  albedoFlag_b = 3,
  diffuse_v3 = 4,
  multiplierMat_b = 5,
  normalFlag_b = 6,
  shininess_f = 7,
  specFlag_b = 8,
  tex_albedo_smp2d = 9,
  tex_normal_smp2d = 10,
  tex_specular_smp2d = 11,
  useDebug_b = 12
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

