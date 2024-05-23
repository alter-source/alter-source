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

LuaHandle* GetLuaHandle();

#endif // MAINLUAHANDLE_H
