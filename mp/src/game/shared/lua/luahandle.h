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

void RegisterConCommand(const char *name, lua_CFunction func);
void RegisterConVar(const char *name, const char *defaultValue, int flags, const char *description);

CBaseLuaHandle *GetLuaHandle();
static lua_State *GetLuaState();