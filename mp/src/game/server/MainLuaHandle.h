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

int luaClientPrintAll(lua_State *L);
int luaMsg(lua_State *L);
int luaConMsg(lua_State *L);
int luaGetTime(lua_State *L);
int luaGetCVar(lua_State *L);
int luaSetCVar(lua_State *L);
int luaLoadFile(lua_State *L);
int luaExecuteConsoleCommand(lua_State *L);

LuaHandle* GetLuaHandle();

#endif // MAINLUAHANDLE_H
