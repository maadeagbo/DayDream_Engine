#include "ddLight.h"
#include "ddTerminal.h"
#include <string>

#define DDLBULB_META_NAME "LuaClass.ddLBulb"
#define check_ddLBulb(L) (ddLBulb **)luaL_checkudata(L, 1, DDLBULB_META_NAME)

const char *ddLBulb_meta_name() { return DDLBULB_META_NAME; }

static int get_id(lua_State *L);
static int set_active(lua_State *L);
static int set_type(lua_State *L);
static int set_pos(lua_State *L);

static int tostring(lua_State *L);

static const struct luaL_Reg lbulb_methods[] = {
    {"id", get_id},       {"set_active", set_active}, {"set_type", set_type},
    {"set_pos", set_pos}, {"__tostring", tostring},   {NULL, NULL}};

void log_meta_ddLBulb(lua_State *L) {
  luaL_newmetatable(L, DDLBULB_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                     /* duplicate the metatable */
  lua_setfield(L, -2, "__index");           /* mt.__index = mt */
  luaL_setfuncs(L, lbulb_methods, 0);       /* register metamethods */
}

int get_id(lua_State *L) {
  ddLBulb *blb = *check_ddLBulb(L);
  lua_pushinteger(L, blb->id);
  return 1;
}

static int set_active(lua_State *L) {
  ddLBulb *blb = *check_ddLBulb(L);

  int top = lua_gettop(L);
  if (top >= 2 && lua_isboolean(L, 2)) {
    blb->active = (bool)lua_toboolean(L, 2);
  } else {
    ddTerminal::post(
        "[error]ddLBulb::set_active::Invalid 1st argument (active : bool)");
  }

  return 0;
}

static int set_type(lua_State *L) {
  ddLBulb *blb = *check_ddLBulb(L);

  int top = lua_gettop(L);
  if (top >= 2 && lua_isstring(L, 2)) {
    string64 type = lua_tostring(L, 2);

    if (type.compare("directional")) {
      blb->type = LightType::DIRECTION_L;
    } else if (type.compare("point")) {
      blb->type = LightType::POINT_L;
    } else if (type.compare("spot")) {
      blb->type = LightType::SPOT_L;
    }
  } else {
    ddTerminal::post(
        "[error]ddLBulb::set_type::Invalid 1st argument (type : string)");
  }

  return 0;
}

static void get_v3_lua(lua_State *L, glm::vec3 &in) {
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
        default:
          break;
      }
    }
  }
}

static int set_pos(lua_State *L) {
  ddLBulb *blb = *check_ddLBulb(L);

  get_v3_lua(L, blb->position);

  return 0;
}

int tostring(lua_State *L) {
  ddLBulb *blb = *check_ddLBulb(L);

  string128 out;
  out.format("ddLBulb(%llu):", (unsigned long long)blb->id);
  
	lua_pushfstring(L, out.str());
  return 1;
}
