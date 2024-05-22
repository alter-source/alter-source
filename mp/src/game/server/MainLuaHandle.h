#ifndef MAINLUAHANDLE_H
#define MAINLUAHANDLE_H

#include "cbase.h"
#include "filesystem.h"
#include "ge_luamanger.h"

extern "C" {
#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lua.hpp"
#include "lua/luaconf.h"
#include "lua/lualib.h"
}

class MainLuaHandle : public LuaHandle {
public:
	MainLuaHandle();
	~MainLuaHandle() {
		Shutdown();
	}

	void Init();
	void Shutdown();
	void RegFunctions();
	void RegGlobals();

private:
	bool m_bLuaLoaded = false;
};

int luaMsg(lua_State *L);
int luaConMsg(lua_State *L);
int luaGetTime(lua_State *L);
int luaGetCVar(lua_State *L);
int luaSetCVar(lua_State *L);
int luaExecuteConsoleCommand(lua_State *L);

int luaGetPlayerName(lua_State *L);
int luaGetPlayerHealth(lua_State *L);
int luaGetPlayerPosition(lua_State *L);
int luaSpawnEntity(lua_State *L);
int luaHandlePlayerInput(lua_State *L);
int luaSay(lua_State *L);

int luaGetPlayerCount(lua_State *L);
int luaGetPlayerTeam(lua_State *L);
int luaGetPlayerMaxSpeed(lua_State *L);
int luaGiveItem(lua_State *L);
int luaIsPlayerValid(lua_State *L);

int luaGetPlayerArmor(lua_State *L);
int luaIsPlayerAlive(lua_State *L);
int luaGetEntityClassname(lua_State *L);
int luaGetEntityModel(lua_State *L);

LuaHandle* GetLuaHandle();

#endif // MAINLUAHANDLE_H
