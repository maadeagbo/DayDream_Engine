#include "ddMaterial.h"
#include <string>

#define DDMAT_META_NAME "LuaClass.ddMat"
#define check_ddMat(L) (ddMat**)luaL_checkudata(L, 1, DDMAT_META_NAME)

const char* ddMat_meta_name() { return DDMAT_META_NAME; }

static int get_id(lua_State* L);
static int set_base_col(lua_State* L);
static int set_spec(lua_State* L);

static int to_string(lua_State* L);

static const struct luaL_Reg mat_methods[] = {{"id", get_id},
                                              {"set_base_color", set_base_col},
                                              {"set_specular", set_spec},
                                              {"__tostring", to_string},
                                              {NULL, NULL}};

void log_meta_ddMat(lua_State* L) {
  luaL_newmetatable(L, DDMAT_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                   /* duplicate the metatable */
  lua_setfield(L, -2, "__index");         /* mt.__index = mt */
  luaL_setfuncs(L, mat_methods, 0);       /* register metamethods */
}

int get_id(lua_State* L) {
  ddMat* mat = *check_ddMat(L);
  lua_pushinteger(L, mat->id);
  return 1;
}

int to_string(lua_State* L) {
  ddMat* mat = *check_ddMat(L);
  std::string buff;

  cbuff<128> out;
  out.format("ddMat(%llu):", (unsigned long long)mat->id);
  buff += out.str();

  // color
  out.format("\n  Base color: %.3f, %.3f, %.3f, %.3f", mat->base_color.r,
             mat->base_color.g, mat->base_color.b, mat->base_color.a);
  buff += out.str();

  // color
  out.format("\n  Specularity: %.3f", mat->spec_value);
  buff += out.str();

  lua_pushstring(L, buff.c_str());
  return 1;
}

static void get_v4_lua(lua_State* L, glm::vec4& in) {
  int top = lua_gettop(L);
  for (unsigned i = 2; i <= (unsigned)top; i++) {
    if (lua_isnumber(L, i)) {
      switch (i) {
        case 2:
          in.x = (float)lua_tonumber(L, i);
          break;
        case 3:
          in.y = (float)lua_tonumber(L, i);
          break;
        case 4:
          in.z = (float)lua_tonumber(L, i);
          break;
        case 5:
          in.w = (float)lua_tonumber(L, i);
          break;
        default:
          break;
      }
    }
  }
}

int set_base_col(lua_State* L) {
  ddMat* mat = *check_ddMat(L);
  get_v4_lua(L, mat->base_color);
  return 0;
}

int set_spec(lua_State* L) {
  ddMat* mat = *check_ddMat(L);

  glm::vec4 spec(0.5f);
  get_v4_lua(L, spec);
  mat->spec_value = spec.r;

  return 0;
}
