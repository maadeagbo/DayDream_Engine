#include "ddLight.h"
#include "ddTerminal.h"
#include <string>

#define DDLBULB_META_NAME "LuaClass.ddLBulb"
#define check_ddLBulb(L) (ddLBulb**)luaL_checkudata(L, 1, DDLBULB_META_NAME)

const char* ddLBulb_meta_name() { return DDLBULB_META_NAME; }

static int get_id(lua_State* L);
static int set_active(lua_State *L);

static int tostring(lua_State* L);

static const struct luaL_Reg lbulb_methods[] = {
	{ "id", get_id },
{"set_active", set_active},
{"__tostring", tostring}, 
{NULL, NULL}};

void log_meta_ddLBulb(lua_State* L) {
  luaL_newmetatable(L, DDLBULB_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                     /* duplicate the metatable */
  lua_setfield(L, -2, "__index");           /* mt.__index = mt */
  luaL_setfuncs(L, lbulb_methods, 0);       /* register metamethods */
}

int get_id(lua_State* L) {
  ddLBulb* blb = *check_ddLBulb(L);
  lua_pushinteger(L, blb->id);
  return 1;
}

static int set_active(lua_State *L) {
	ddLBulb* blb = *check_ddLBulb(L);

	int top = lua_gettop(L);
	if (top >= 2 && lua_isboolean(L, 2)) {
		blb->active = (bool)lua_toboolean(L, 2);
	} else {
		ddTerminal::post("[error]ddLBulb::set_active::Invalid 1st argument (active : bool)");
	}
	
	return 0;
}

int tostring(lua_State* L) {
  ddLBulb* blb = *check_ddLBulb(L);
  std::string buff;

  cbuff<128> out;
  out.format("ddLBulb(%llu):", (unsigned long long)blb->id);
  buff += out.str();

  return 0;
}
