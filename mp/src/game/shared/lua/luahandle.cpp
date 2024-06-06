#include "cbase.h"
#include "luahandle.h"
#include "filesystem.h"

#ifndef CLIENT_DLL
#include "hl2_player.h"
#include "hl2mp_player.h"
#else
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"

using namespace vgui;
#endif

// Get the LuaHandle instance
// This will be initialized in CGameRules::CGameRules()
static CBaseLuaHandle *g_LuaHandle = NULL;

LuaHandle::LuaHandle() {
	g_LuaHandle = this;
	Register();
}

LuaHandle::~LuaHandle() {
}

void LuaHandle::Init() {
#ifndef CLIENT_DLL
	const char *luaFile = "lua/init.lua";
#else
	const char *luaFile = "lua/cl/init.lua";
#endif

	// Load into buffer
	FileHandle_t f = filesystem->Open(luaFile, "rb", "MOD");
	if (!f)
		return;

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	unsigned bufSize =
		((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 1);

	char *buffer =
		(char *)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
	Assert(buffer);

	((IFileSystem *)filesystem)
		->ReadEx(buffer, bufSize, fileSize, f);  // read into local buffer
	buffer[fileSize] = '\0';                     // null terminate file as EOF
	filesystem->Close(f);                        // close file after reading

	int error = luaL_loadbuffer(GetLua(), buffer, fileSize, luaFile);
	if (error) {
		Warning("[LUA-ERR] %s\n", lua_tostring(GetLua(), -1));
		lua_pop(GetLua(), 1); /* pop error message from the stack */
		Warning(
			"[LUA-ERR] One or more errors occured while loading lua "
			"script!\n");
		return;
	}
	CallLua(GetLua(), 0, LUA_MULTRET, 0, luaFile);
	m_bLuaLoaded = true;
}

void LuaHandle::LoadLua(const char *luaFile) {
	if (!GetLuaHandle())
	{
		Warning("cannot find Lua handle");
		return;
	}

	// Load into buffer
	FileHandle_t f = filesystem->Open(luaFile, "rb", "MOD");
	if (!f)
		return;

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	unsigned bufSize =
		((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 1);

	char *buffer =
		(char *)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
	Assert(buffer);

	((IFileSystem *)filesystem)
		->ReadEx(buffer, bufSize, fileSize, f);  // read into local buffer
	buffer[fileSize] = '\0';                     // null terminate file as EOF
	filesystem->Close(f);                        // close file after reading

	int error = luaL_loadbuffer(GetLua(), buffer, fileSize, luaFile);
	if (error) {
		Warning("[LUA-ERR] %s\n", lua_tostring(GetLua(), -1));
		lua_pop(GetLua(), 1); /* pop error message from the stack */
		Warning(
			"[LUA-ERR] One or more errors occured while loading lua "
			"script!\n");
		return;
	}
	CallLua(GetLua(), 0, LUA_MULTRET, 0, luaFile);
	m_bLuaLoaded = true;
}

void LuaHandle::Shutdown() {
}

void LuaHandle::RegisterGlobals() {
	LG_DEFINE_INT("FOR_ALL_PLAYERS", -1);
	LG_DEFINE_INT("INVALID_ENTITY", -1);
	LG_DEFINE_INT("NULL", 0);
	LG_DEFINE_STRING("GAMEMODE", cvar->FindVar("as_gamemode")->GetString());
	LG_DEFINE_INT("TEAMPLAY", cvar->FindVar("mp_teamplay")->GetInt());
	LG_DEFINE_INT("COOP", cvar->FindVar("coop")->GetInt());

#ifndef CLIENT_DLL
	LG_DEFINE_INT("WEAPON_MELEE_SLOT", WEAPON_MELEE_SLOT);
	LG_DEFINE_INT("WEAPON_SECONDARY_SLOT", WEAPON_SECONDARY_SLOT);
	LG_DEFINE_INT("WEAPON_PRIMARY_SLOT", WEAPON_PRIMARY_SLOT);
	LG_DEFINE_INT("WEAPON_EXPLOSIVE_SLOT", WEAPON_EXPLOSIVE_SLOT);
	LG_DEFINE_INT("WEAPON_TOOL_SLOT", WEAPON_TOOL_SLOT);
#endif

	LG_DEFINE_INT("MAX_FOV", MAX_FOV);
	LG_DEFINE_INT("MAX_HEALTH", 100);
	LG_DEFINE_INT("MAX_ARMOR", 200);
	LG_DEFINE_INT("MAX_PLAYERS", gpGlobals->maxClients);

	LG_DEFINE_INT("TEAM_NONE", TEAM_UNASSIGNED);
	LG_DEFINE_INT("TEAM_SPECTATOR", TEAM_SPECTATOR);
	LG_DEFINE_INT("TEAM_ANY", TEAM_ANY);
	LG_DEFINE_INT("TEAM_INVALID", TEAM_INVALID);

#ifndef CLIENT_DLL
	LG_DEFINE_INT("TEAM_ONE", TEAM_COMBINE);
	LG_DEFINE_INT("TEAM_TWO", TEAM_REBELS);
#endif

	LG_DEFINE_INT("HUD_PRINTNOTIFY", HUD_PRINTNOTIFY);
	LG_DEFINE_INT("HUD_PRINTCONSOLE", HUD_PRINTCONSOLE);
	LG_DEFINE_INT("HUD_PRINTTALK", HUD_PRINTTALK);
	LG_DEFINE_INT("HUD_PRINTCENTER", HUD_PRINTCENTER);
}

// Note:
// All functions that are registered in "RegisterFunctions" must have their
// actual function definition names prefixed with "lua".
void LuaHandle::RegisterFunctions() {
	REG_FUNCTION(Msg);
	REG_FUNCTION(CurTime);
	REG_FUNCTION(MaxPlayers);
	REG_FUNCTION(GetModPath);
	REG_FUNCTION(GetConVar_Float);
	REG_FUNCTION(GetConVar_String);
	REG_FUNCTION(GetConVar_Bool);
#ifndef CLIENT_DLL
	REG_FUNCTION(StartNextLevel);
	REG_FUNCTION(ServerCommand);
	REG_FUNCTION(BroadcastMessage);
	REG_FUNCTION(IsDedicatedServer);
	REG_FUNCTION(GetCurrentMap);
	REG_FUNCTION(KickPlayer);
	REG_FUNCTION(BanPlayer);
#endif
#ifndef CLIENT_DLL
	REG_FUNCTION(GiveItem);
	REG_FUNCTION(GiveAmmo);
#endif
	REG_FUNCTION(SetHealth);
	REG_FUNCTION(GetHealth);
	REG_FUNCTION(GetPlayerName);
#ifndef CLIENT_DLL
	REG_FUNCTION(TeleportPlayer);
#endif
	REG_FUNCTION(GetPlayerPosition);
	REG_FUNCTION(GetPlayerTeam);
	REG_FUNCTION(FileExists);
	REG_FUNCTION(FileRead);
	REG_FUNCTION(FileWrite);
	REG_FUNCTION(CreateDir);
	REG_FUNCTION(IsDir);
	REG_FUNCTION(DeleteFile);
	REG_FUNCTION(RenameFile);
#ifndef CLIENT_DLL
	REG_FUNCTION(SpawnEntity);
	REG_FUNCTION(RemoveEntity);
	REG_FUNCTION(TriggerGameEvent);
	REG_FUNCTION(LogToFile);
	REG_FUNCTION(EntityExists);
#endif
#ifdef CLIENT_DLL
	REG_FUNCTION(PlaySound);
#endif
#ifndef CLIENT_DLL
	REG_FUNCTION(IsDead);
#endif
	REG_FUNCTION(IsLinux);
}

#define LUA_FUNC(name, func) int name(lua_State *L) { return func(L); }

#pragma warning(disable: 4238)
#pragma warning(disable: 4800)
#pragma warning(disable: 4189)
#pragma warning(disable: 4700)
// Important

// server-related
LUA_FUNC(luaMsg, [](lua_State * L) {
	Msg("%s\n", lua_tostring(L, 1));
	return 0;
})

LUA_FUNC(luaCurTime, [](lua_State * L) {
	lua_pushnumber(L, gpGlobals->curtime);
	return 1;
})

LUA_FUNC(luaMaxPlayers, [](lua_State * L) {
	lua_pushnumber(L, gpGlobals->maxClients);
	return 1;
})

LUA_FUNC(luaGetModPath, [](lua_State *L) {
	char modPath[MAX_PATH];
	if (filesystem->GetCurrentDirectory(modPath, sizeof(modPath))) {
		lua_pushstring(L, modPath);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
})

LUA_FUNC(luaGetConVar_Float, [](lua_State *L) {
	const char* name = lua_tostring(L, 1);
	if (name) {
		ConVar* conVar = cvar->FindVar(name);
		if (conVar && !conVar->IsFlagSet(FCVAR_SERVER_CANNOT_QUERY)) {
			lua_pushnumber(L, conVar->GetFloat());
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
})

LUA_FUNC(luaGetConVar_String, [](lua_State *L) {
	const char* name = lua_tostring(L, 1);
	if (name) {
		ConVar* conVar = cvar->FindVar(name);
		if (conVar && !conVar->IsFlagSet(FCVAR_SERVER_CANNOT_QUERY)) {
			lua_pushstring(L, conVar->GetString());
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
})

LUA_FUNC(luaGetConVar_Bool, [](lua_State *L) {
	const char* name = lua_tostring(L, 1);
	if (name) {
		ConVar* conVar = cvar->FindVar(name);
		if (conVar && !conVar->IsFlagSet(FCVAR_SERVER_CANNOT_QUERY)) {
			lua_pushboolean(L, conVar->GetBool());
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
})

#ifndef CLIENT_DLL
LUA_FUNC(luaStartNextLevel, [](lua_State *L) {
	const char* mapName = lua_tostring(L, 1);
	if (mapName) {
		engine->ChangeLevel(mapName, nullptr);
	}
	return 0;
})

LUA_FUNC(luaServerCommand, [](lua_State *L) {
	engine->ServerCommand(lua_tostring(L, 1));
	engine->ServerExecute();
	return 0;
})

LUA_FUNC(luaBroadcastMessage, [](lua_State *L) {
	const char* message = lua_tostring(L, 1);
	if (message) {
		UTIL_ClientPrintAll(HUD_PRINTTALK, message);
	}
	return 0;
})

LUA_FUNC(luaIsDedicatedServer, [](lua_State *L) {
	lua_pushboolean(L, engine->IsDedicatedServer());
	return 1;
})

LUA_FUNC(luaGetCurrentMap, [](lua_State *L) {
	const char* currentMap = STRING(gpGlobals->mapname);
	lua_pushstring(L, currentMap);
	return 1;
})

LUA_FUNC(luaKickPlayer, [](lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	const char* reason = lua_tostring(L, 2);

	if (auto p = UTIL_PlayerByIndex(playerIndex)) {
		engine->ServerCommand(UTIL_VarArgs("kickid %d %s\n", p->GetUserID(), reason));
	}
	return 0;
})

LUA_FUNC(luaBanPlayer, [](lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	const char* duration = lua_tostring(L, 2); // duration in minutes, or "permanent" for permanent ban
	const char* reason = lua_tostring(L, 3);

	if (auto p = UTIL_PlayerByIndex(playerIndex)) {
		if (Q_stricmp(duration, "permanent") == 0) {
			engine->ServerCommand(UTIL_VarArgs("banid 0 %d kick %s\n", p->GetUserID(), reason));
		}
		else {
			engine->ServerCommand(UTIL_VarArgs("banid %s %d kick %s\n", duration, p->GetUserID(), reason));
		}
		engine->ServerCommand("writeid\n");
	}
	return 0;
})

#endif

// player-related
#ifndef CLIENT_DLL
LUA_FUNC(luaGiveItem, [](lua_State * L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) p->GiveNamedItem(lua_tostring(L, 2));
	return 0;
})
LUA_FUNC(luaGiveAmmo, [](lua_State * L) {
	int playerIndex = lua_tointeger(L, 1);
	const char * ammoType = lua_tostring(L, 2);
	int amount = lua_tointeger(L, 3);
	if (!UTIL_PlayerByIndex(playerIndex)) {
		lua_pushstring(L, "Invalid player index");
		return 1;
	}
	UTIL_PlayerByIndex(playerIndex)->GiveAmmo(amount, ammoType);
	return 0;
})
#endif

LUA_FUNC(luaSetHealth, [](lua_State *L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		p->SetHealth(lua_tointeger(L, 2));
	}
	return 0;
})

LUA_FUNC(luaGetHealth, [](lua_State *L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		lua_pushinteger(L, p->GetHealth());
		return 1;
	}
	lua_pushnil(L);
	return 1;
})

LUA_FUNC(luaGetPlayerName, [](lua_State *L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		lua_pushstring(L, p->GetPlayerName());
		return 1;
	}
	lua_pushnil(L);
	return 1;
})

#ifndef CLIENT_DLL
LUA_FUNC(luaTeleportPlayer, [](lua_State *L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		Vector position;
		position.x = lua_tonumber(L, 2);
		position.y = lua_tonumber(L, 3);
		position.z = lua_tonumber(L, 4);
		p->Teleport(&position, nullptr, nullptr);
	}
	return 0;
})
#endif

LUA_FUNC(luaGetPlayerPosition, [](lua_State *L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		Vector position = p->GetAbsOrigin();
		lua_newtable(L);
		lua_pushnumber(L, position.x);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, position.y);
		lua_setfield(L, -2, "y");
		lua_pushnumber(L, position.z);
		lua_setfield(L, -2, "z");
		return 1;
	}
	lua_pushnil(L);
	return 1;
})
LUA_FUNC(luaGetPlayerTeam, [](lua_State *L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		lua_pushinteger(L, p->GetTeamNumber());
		return 1;
	}
	lua_pushnil(L);
	return 1;
})

// file manipulation
LUA_FUNC(luaFileExists, [](lua_State *L) {
	const char* filename = lua_tostring(L, 1);
	if (filename && filesystem->FileExists(filename)) {
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(luaFileRead, [](lua_State *L) {
	const char* filename = lua_tostring(L, 1);
	if (filename && filesystem->FileExists(filename)) {
		FileHandle_t f = filesystem->Open(filename, "rb", "MOD");
		if (f) {
			int fileSize = filesystem->Size(f);
			char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, fileSize + 1);
			Assert(buffer);
			((IFileSystem *)filesystem)->ReadEx(buffer, fileSize + 1, fileSize, f);
			buffer[fileSize] = '\0';
			filesystem->Close(f);
			lua_pushstring(L, buffer);
			((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
})

LUA_FUNC(luaFileWrite, [](lua_State *L) {
	const char* filename = lua_tostring(L, 1);
	const char* content = lua_tostring(L, 2);
	if (filename && content) {
		FileHandle_t f = filesystem->Open(filename, "wb", "MOD");
		if (f) {
			filesystem->Write(content, strlen(content), f);
			filesystem->Close(f);
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
})

LUA_FUNC(luaCreateDir, [](lua_State *L) {
	const char* folder = lua_tostring(L, 1);
	if (folder && !filesystem->IsDirectory(folder)) {
		filesystem->CreateDirHierarchy(folder, "MOD");
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(luaIsDir, [](lua_State *L) {
	const char* folder = lua_tostring(L, 1);
	if (folder && filesystem->IsDirectory(folder)) {
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(luaDeleteFile, [](lua_State *L) {
	const char* filename = lua_tostring(L, 1);
	if (filename && filesystem->FileExists(filename)) {
		filesystem->RemoveFile(filename, "MOD");
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(luaRenameFile, [](lua_State *L) {
	const char* before = lua_tostring(L, 1);
	const char* after = lua_tostring(L, 2);
	if (before && after && filesystem->FileExists(before)) {
		filesystem->RenameFile(before, after, "MOD");
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

// entity-related
#ifndef CLIENT_DLL
LUA_FUNC(luaSpawnEntity, [](lua_State *L) {
	const char* entityName = lua_tostring(L, 1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float z = lua_tonumber(L, 4);

	if (entityName) {
		Vector position(x, y, z);
		CBaseEntity* entity = CreateEntityByName(entityName);
		if (entity) {
			entity->SetAbsOrigin(position);
			DispatchSpawn(entity);
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
})

LUA_FUNC(luaRemoveEntity, [](lua_State *L) {
	const char* entityName = lua_tostring(L, 1);
	int entityIndex = lua_tointeger(L, 2);

	CBaseEntity* entity = nullptr;
	if (entityName) {
		entity = gEntList.FindEntityByName(nullptr, entityName);
	}
	else if (entityIndex > 0) {
		entity = UTIL_EntityByIndex(entityIndex);
	}

	if (entity) {
		UTIL_Remove(entity);
		lua_pushboolean(L, true);
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
})

LUA_FUNC(luaTriggerGameEvent, [](lua_State *L) {
	const char* eventName = lua_tostring(L, 1);
	if (eventName) {
		IGameEvent *event = gameeventmanager->CreateEvent(eventName);
		if (event) {
			gameeventmanager->FireEvent(event);
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
})

LUA_FUNC(luaLogToFile, [](lua_State *L) {
	const char* filename = lua_tostring(L, 1);
	const char* message = lua_tostring(L, 2);

	if (filename && message) {
		FileHandle_t f = filesystem->Open(filename, "a", "MOD");
		if (f) {
			filesystem->FPrintf(f, "%s\n", message);
			filesystem->Close(f);
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
})

LUA_FUNC(luaEntityExists, [](lua_State *L) {
	const char* entityName = lua_tostring(L, 1);
	int entityIndex = lua_tointeger(L, 2);

	CBaseEntity* entity = nullptr;
	if (entityName) {
		entity = gEntList.FindEntityByName(nullptr, entityName);
	}
	else if (entityIndex > 0) {
		entity = UTIL_EntityByIndex(entityIndex);
	}

	if (entity) {
		lua_pushboolean(L, true);
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
})
#endif

// sounds
#ifdef CLIENT_DLL
LUA_FUNC(luaPlaySound, [](lua_State *L) {
	const char* soundName = lua_tostring(L, 1);
	surface()->PlaySound(soundName);
	return 0;
})
#endif

// event

#ifndef CLIENT_DLL
LUA_FUNC(luaIsDead, [](lua_State *L) {
	CHL2MP_Player* pPlayer = ToHL2MPPlayer(pPlayer);
	if (pPlayer->IsDead())
	{
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushboolean(L, false);
	}
	return 1;
})
#endif

// other
LUA_FUNC(luaIsLinux, [](lua_State *L) {
	lua_pushboolean(L, false); // checks if the game is ran on Linux or shit.
	return 1;
})

CBaseLuaHandle *GetLuaHandle() {
	Assert(g_LuaHandle);

	return g_LuaHandle;
}

// Handy for getting the state in one go.
// Use GetLuaHandle if more control is needed.
lua_State *GetLuaState() {
	CBaseLuaHandle *luaHandle = GetLuaHandle();

	return luaHandle->GetLua();
}

static bool RunLuaString(const char *string) {
	lua_State *L = GetLuaState();

	int error = luaL_loadstring(L, string);

	if (error) {
		Warning("[LUA-ERR] %s\n", lua_tostring(L, -1));
		lua_pop(L, 1); /* pop error message from the stack */
		return false;
	}

	CallLua(L, 0, LUA_MULTRET, 0, string);
	return true;
}

#ifdef CLIENT_DLL
CON_COMMAND(lua_run_cl, "Run a Lua string in the client realm") {
	RunLuaString(args.ArgS());
}
#else
CON_COMMAND(lua_run, "Run a Lua string in the server realm") {
	if (!UTIL_IsCommandIssuedByServerAdmin()) {
		cvar->ConsoleColorPrintf(LUA_PROHIBIT_PRINT_COLOUR,
			"[LUA-ERR] Admin-only command!\n");
		return;
	}

	RunLuaString(args.ArgS());
}
#endif