#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Text:
*		- uses FreeType Fonts to use words in the engine
*
*	TODO:	==
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "DD_Shader.h"

struct DD_Char {
  GLuint m_TextureID;
  glm::ivec2 _size;
  glm::ivec2 m_Bearing;
  GLuint m_advance;
};

struct DD_Text {
  std::string font;
  std::map<GLchar, DD_Char> Characters;
};

namespace TextSpace {
DD_Text InitFontLib(const char* font_type, const int _w, const int _h,
                    float scrW, float scrH);
void RenderText(DD_Shader* shader, DD_Text& ttf, std::string text, GLfloat x,
                GLfloat y, GLfloat scale, glm::vec3 color);
}
