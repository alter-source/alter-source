#include "cbase.h"
#include "baseluahandle.h"

class LuaHandle : CBaseLuaHandle {
public:
	LuaHandle();
	~LuaHandle();

	void Init();
	void Shutdown();
	void RegisterFunctions();
	void RegisterGlobals();

	void LoadLua(const char *luaFile);
};

CBaseLuaHandle *GetLuaHandle();
static lua_State *GetLuaState();