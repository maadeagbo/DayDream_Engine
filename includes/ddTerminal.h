#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Interface:
*		- uses dear IMGUI for interface
*
*	TODO:	==
*
-----------------------------------------------------------------------------*/

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include "ddIncludes.h"
#include "ddInput.h"
#include "ddTimer.h"

#ifdef __linux__
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

enum class TerminalCmds : unsigned { NULL_CMD = 0x0, RENDER_DEBUG = 0x1 };
ENABLE_BITMASK_OPERATORS(TerminalCmds)

namespace ddTerminal {
void flipDebugFlag();
void post(const char* message);
void clearTerminal();
void display(const float scr_width, const float scr_height);
void dumpTerminalToImGuiText();
const char* pollBuffer();
void get_input(DD_LEvent& _event);
void inTerminalHistory();
void outTerminalHistory();

template <typename... Args>
void f_post(const char* format_str, const Args&... args) {
  char buff[512];
  snprintf(buff, sizeof(buff), format_str, args...);
  post(buff);
}
}

int script_print(lua_State* L);
