#include "DD_Text.h"

namespace {
GLuint quadVAO = 0, quadVBO = 0;
glm::mat4 projMat;
}

// Load font
// _w: font width (0 means set width dynamically based on height)
// _h: font height
DD_Text TextSpace::InitFontLib(const char *font_type, const int _w,
                               const int _h, float scrW, float scrH) {
  DD_Text fontLib = DD_Text();
  FT_Library ft_lib;
  FT_Face ft_face;

  if (FT_Init_FreeType(&ft_lib)) {
    printf("ERROR::FREETYPE: Could not init FreeType Library\n");
  }

  std::string font = std::string(FONTS_DIR) + font_type + ".ttf";
  if (FT_New_Face(ft_lib, font.c_str(), 0, &ft_face)) {
    printf("ERROR::FREETYPE: Failed to load %s font.\n", font_type);
  }

  FT_Set_Pixel_Sizes(ft_face, _w, _h);
  // load up char array of textures
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Disable byte-alignment restriction

  for (GLubyte c = 0; c < 128; c++) {
    // Load character glyph
    if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER)) {
      printf("ERROR::FREETYTPE: Failed to load Glyph");
      continue;
    }
    // Generate texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ft_face->glyph->bitmap.width,
                 ft_face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                 ft_face->glyph->bitmap.buffer);
    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // store character for later use
    DD_Char character = {
        texture,
        glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
        glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top),
        (GLuint)ft_face->glyph->advance.x};
    fontLib.Characters[GLchar(c)] = character;
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // Restore byte-alignment restriction
  // clean up resources
  FT_Done_Face(ft_face);
  FT_Done_FreeType(ft_lib);

  // set projection matrix
  projMat = glm::ortho(0.0f, scrW, 0.0f, scrH);

  return fontLib;
}

void TextSpace::RenderText(DD_Shader *shader, DD_Text &ttf, std::string text,
                           GLfloat x, GLfloat y, GLfloat scale,
                           glm::vec3 color) {
  if (quadVAO == 0) {
    // Setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          (GLvoid *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  shader->Use();
  shader->setUniform("textColor", color);
  shader->setUniform("projection", projMat);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(quadVAO);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Iterate through all characters
  std::string::const_iterator c;
  for (c = text.begin(); c != text.end(); c++) {
    DD_Char ch = ttf.Characters[*c];

    GLfloat xpos = x + ch.m_Bearing.x * scale;
    GLfloat ypos = y - (ch._size.y - ch.m_Bearing.y) * scale;

    GLfloat w = ch._size.x * scale;
    GLfloat h = ch._size.y * scale;
    // Update VBO for each character
    GLfloat vertices[6][4] = {
        {xpos, ypos + h, 0.0, 0.0},    {xpos, ypos, 0.0, 1.0},
        {xpos + w, ypos, 1.0, 1.0},

        {xpos, ypos + h, 0.0, 0.0},    {xpos + w, ypos, 1.0, 1.0},
        {xpos + w, ypos + h, 1.0, 0.0}};
    // Render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, ch.m_TextureID);
    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Now advance cursors for next glyph
    // (note that advance is number of 1/64 pixels)
    // Bitshift by 6 to get value in pixels (2^6 = 64)
    x += (ch.m_advance >> 6) * scale;
  }
  glDisable(GL_BLEND);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}
