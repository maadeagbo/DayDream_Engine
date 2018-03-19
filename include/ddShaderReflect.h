// Enums for managing shader uniforms

enum class RE_Light : int {
  Proj_m4x4 = 0,
  View_m4x4 = 1,
  Model_m4x4 = 2,
  DrawSky_b = 3,
  DepthTex_smp2d = 4,
  Light_position_v3 = 5,
  Light_direction_v3 = 6,
  Light_color_v3 = 7,
  Light_type_i = 8,
  Light_cutoff_i_f = 9,
  Light_cutoff_o_f = 10,
  Light_lumin_cutoff_f = 12,
  screenDimension_v2 = 16,
  PositionTex_smp2d = 17,
  NormalTex_smp2d = 18,
  ColorTex_smp2d = 19,
  viewPos_v3 = 20,
  skybox_smpCube = 21,
  ShadowMap_b = 22,
  LSM_m4x4 = 23,
  Debug_b = 24,
  Debug_Color_v4 = 25
};

enum class RE_GBuffer : int {
  enable_clip1_b = 0,
  multiplierMat_b = 1,
  Norm_m4x4 = 2,
  Model_m4x4 = 3,
  VP_m4x4 = 4,
  diffuse_v3 = 5,
  shininess_f = 6,
  albedoFlag_b = 7,
  specFlag_b = 8,
  normalFlag_b = 9,
  useDebug_b = 10,
  tex_normal_smp2d = 11,
  tex_albedo_smp2d = 12,
  tex_specular_smp2d = 13
};

enum class RE_PostPr : int {
  SampleShadow_b = 0,
  flip_y_coord_b = 1,
  flip_x_coord_b = 2,
  uv_rotate_b = 3,
  rotate_uv_mat_m4x4 = 4,
  LSM_m4x4 = 5,
  MVP_m4x4 = 6,
  rgb2xyz_m3x3 = 7,
  xyz2rgb_m3x3 = 8,
  DoToneMap_b = 9,
  SampleMap_b = 10,
  Blur_b = 11,
  GammaCorrect_b = 12,
  BlendParticle_b = 13,
  output2D_b = 14,
  ColorTex_smp2d = 15,
  Exposure_f = 16,
  AveLum_f = 17,
  White_f = 18,
  direction_flag_v2 = 19,
  ParticleTex_smp2d = 20
};

enum class RE_Text : int {
  projection_m4x4 = 0,
  text_smp2d = 1,
  textColor_v3 = 2
};

enum class RE_PingP : int { ColorTex_smp2d = 0 };

enum class RE_Lumin : int {
  img_input_img2D = 0,
  computeLum_b = 1,
  img_output_img2D = 2
};

enum class RE_Line : int { MVP_m4x4 = 0, color_v4 = 1 };

enum class RE_LightSt : int { MVP_m4x4 = 0 };

enum class RE_Shadow : int { LightSpace_m4x4 = 0, MVP_m4x4 = 1 };

enum class RE_ShadowSk : int { LightSpace_m4x4 = 0, MVP_m4x4 = 1 };
