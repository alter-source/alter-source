#include "cbase.h"
#include "MainLuaHandle.h"
#include "filesystem.h"
#include "convar.h"
#include <stdio.h>
#include "tier0/memdbgon.h"

#include "hl2_player.h"
#include "hl2mp_player.h"

void CC_ReloadLua(const CCommand& args) {
	if (GetLuaHandle()) {
		GetLuaHandle()->Init();
	}
	else {
		ConMsg("Lua handle not available.\n");
	}
}
static ConCommand reload_lua("reload_lua", CC_ReloadLua, "Reload Lua scripts.", FCVAR_NONE);

void MainLuaHandle::Init() {
	const char* luaFolder = "lua/";
	const char* luaFile = "main.lua";
	char fullPath[MAX_PATH];

	Q_snprintf(fullPath, sizeof(fullPath), "%s%s", luaFolder, luaFile);

	FileHandle_t f = filesystem->Open(fullPath, "rb", "MOD");
	if (!f) {
		Warning("[LUA-ERR] Failed to open %s\n", fullPath);
		return;
	}

	int fileSize = filesystem->Size(f);
	unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 1);

	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
	Assert(buffer);

	((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f);
	buffer[fileSize] = '\0';
	filesystem->Close(f);

	int error = luaL_loadbuffer(GetLua(), buffer, fileSize, fullPath);
	if (error) {
		Warning("[LUA-ERR] %s\n", lua_tostring(GetLua(), -1));
		lua_pop(GetLua(), 1);
		Warning("[LUA-ERR] One or more errors occurred while loading Lua script!\n");
		((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
		return;
	}
	CallLUA(GetLua(), 0, LUA_MULTRET, 0, fullPath);
	m_bLuaLoaded = true;

	((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
}

void MainLuaHandle::RegGlobals() {
	LG_DEFINE_INT("FOR_ALL_PLAYERS", -1);
	LG_DEFINE_INT("INVALID_ENTITY", -1);
	LG_DEFINE_INT("NULL", 0);
	LG_DEFINE_STRING("GAMEMODE", cvar->FindVar("as_gamemode")->GetString());
	LG_DEFINE_INT("TEAMPLAY", cvar->FindVar("mp_teamplay")->GetInt());
	LG_DEFINE_INT("COOP", cvar->FindVar("coop")->GetInt());

	LG_DEFINE_INT("WEAPON_MELEE_SLOT", WEAPON_MELEE_SLOT);
	LG_DEFINE_INT("WEAPON_SECONDARY_SLOT", WEAPON_SECONDARY_SLOT);
	LG_DEFINE_INT("WEAPON_PRIMARY_SLOT", WEAPON_PRIMARY_SLOT);
	LG_DEFINE_INT("WEAPON_EXPLOSIVE_SLOT", WEAPON_EXPLOSIVE_SLOT);
	LG_DEFINE_INT("WEAPON_TOOL_SLOT", WEAPON_TOOL_SLOT);

	LG_DEFINE_INT("MAX_FOV", MAX_FOV);
	LG_DEFINE_INT("MAX_HEALTH", 100);
	LG_DEFINE_INT("MAX_ARMOR", 200);
	LG_DEFINE_INT("MAX_PLAYERS", gpGlobals->maxClients);

	LG_DEFINE_INT("TEAM_NONE", TEAM_UNASSIGNED);
	LG_DEFINE_INT("TEAM_SPECTATOR", TEAM_SPECTATOR);
	LG_DEFINE_INT("TEAM_ANY", TEAM_ANY);
	LG_DEFINE_INT("TEAM_INVALID", TEAM_INVALID);

	LG_DEFINE_INT("TEAM_ONE", TEAM_COMBINE);
	LG_DEFINE_INT("TEAM_TWO", TEAM_REBELS);

	LG_DEFINE_INT("HUD_PRINTNOTIFY", HUD_PRINTNOTIFY);
	LG_DEFINE_INT("HUD_PRINTCONSOLE", HUD_PRINTCONSOLE);
	LG_DEFINE_INT("HUD_PRINTTALK", HUD_PRINTTALK);
	LG_DEFINE_INT("HUD_PRINTCENTER", HUD_PRINTCENTER);
}

void MainLuaHandle::Shutdown() {
	if (!m_bLuaLoaded) {
		Warning("[LUA-INFO] Lua not loaded, nothing to shutdown.\n");
		return;
	}

	m_bLuaLoaded = false;

	ConMsg("[LUA-INFO] Lua shutdown successfully.\n");
}

int luaMsg(lua_State *L) {
	Msg("%s\n", lua_tostring(L, 1));
	return 0;
}

int luaConMsg(lua_State *L) {
	ConMsg("%s\n", lua_tostring(L, 1));
	return 0;
}

int luaGetTime(lua_State *L) {
	lua_pushnumber(L, gpGlobals->curtime);
	return 1;
}

int luaGetCVar(lua_State *L) {
	const char* cvarName = lua_tostring(L, 1);
	ConVar* cvare = cvar->FindVar(cvarName);
	lua_pushstring(L, cvare ? cvare->GetString() : nullptr);
	return 1;
}

int luaSetCVar(lua_State *L) {
	const char* cvarName = lua_tostring(L, 1);
	const char* cvarValue = lua_tostring(L, 2);
	ConVar* cvare = cvar->FindVar(cvarName);
	if (cvare)
		cvare->SetValue(cvarValue);
	return 0;
}

int luaExecuteConsoleCommand(lua_State *L) {
	const char* command = lua_tostring(L, 1);
	if (command) {
		engine->ServerCommand(command);
		engine->ServerExecute();
	}
	return 0;
}

int luaGetPlayerName(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	lua_pushstring(L, pPlayer ? pPlayer->GetPlayerName() : nullptr);
	return 1;
}

int luaGetPlayerHealth(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	lua_pushinteger(L, pPlayer ? pPlayer->GetHealth() : -1);
	return 1;
}

int luaGetPlayerPosition(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	if (pPlayer) {
		Vector position = pPlayer->GetAbsOrigin();
		lua_pushnumber(L, position.x);
		lua_pushnumber(L, position.y);
		lua_pushnumber(L, position.z);
	}
	else {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}

int luaHandlePlayerInput(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	const char *input = lua_tostring(L, 2);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	if (pPlayer && input) {
		if (strcmp(input, "jump") == 0)
			pPlayer->Jump();
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
}

int luaSay(lua_State *L) {
	int n = lua_gettop(L);
	if (n < 1) {
		luaL_error(L, "luaSay expects at least 1 argument");
		return 0;
	}
	std::string message;
	for (int i = 1; i <= n; ++i) {
		message += lua_tostring(L, i);
		if (i < n) message += " ";
	}
	UTIL_ClientPrintAll(HUD_PRINTTALK, message.c_str());
	return 0;
}

int luaGetEntityPosition(lua_State *L) {
	int entityIndex = lua_tointeger(L, 1);
	CBaseEntity *pEntity = UTIL_EntityByIndex(entityIndex);
	if (pEntity) {
		Vector position = pEntity->GetAbsOrigin();
		lua_pushnumber(L, position.x);
		lua_pushnumber(L, position.y);
		lua_pushnumber(L, position.z);
	}
	else {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}


int luaGetPlayerCount(lua_State *L) {
	lua_pushinteger(L, gpGlobals->maxClients);
	return 1;
}

int luaGetPlayerTeam(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	lua_pushinteger(L, pPlayer ? pPlayer->GetTeamNumber() : -1);
	return 1;
}

int luaGetPlayerMaxSpeed(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	lua_pushinteger(L, pPlayer ? pPlayer->GetPlayerMaxSpeed() : -1);
	return 1;
}

int luaGiveItem(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	const char *itemName = lua_tostring(L, 2);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	if (pPlayer && itemName) {
		pPlayer->GiveNamedItem(itemName);
	}
	return 0;
}

int luaIsPlayerValid(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	lua_pushboolean(L, UTIL_PlayerByIndex(playerIndex) != NULL);
	return 1;
}

int luaGetPlayerArmor(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	lua_pushinteger(L, pPlayer ? pPlayer->ArmorValue() : -1);
	return 1;
}

int luaIsPlayerAlive(lua_State *L) {
	int playerIndex = lua_tointeger(L, 1);
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);
	lua_pushboolean(L, pPlayer && pPlayer->IsAlive());
	return 1;
}

int luaGetEntityClassname(lua_State *L) {
	int entityIndex = lua_tointeger(L, 1);
	CBaseEntity *pEntity = UTIL_EntityByIndex(entityIndex);
	lua_pushstring(L, pEntity ? pEntity->GetClassname() : nullptr);
	return 1;
}

int luaGetEntityModel(lua_State *L) {
	int entityIndex = lua_tointeger(L, 1);
	CBaseEntity *pEntity = UTIL_EntityByIndex(entityIndex);
	lua_pushstring(L, pEntity ? pEntity->GetModelName() : nullptr);
	return 1;
}


void MainLuaHandle::RegFunctions() {
	REG_FUNCTION(Msg);
	REG_FUNCTION(ConMsg);
	REG_FUNCTION(GetTime);
	REG_FUNCTION(GetCVar);
	REG_FUNCTION(SetCVar);
	REG_FUNCTION(ExecuteConsoleCommand);

	REG_FUNCTION(GetPlayerName);
	REG_FUNCTION(GetPlayerHealth);
	REG_FUNCTION(GetPlayerPosition);
	REG_FUNCTION(HandlePlayerInput);
	REG_FUNCTION(Say);

	REG_FUNCTION(GetPlayerCount);
	REG_FUNCTION(GetPlayerTeam);
	REG_FUNCTION(GetPlayerMaxSpeed);
	REG_FUNCTION(GiveItem);
	REG_FUNCTION(IsPlayerValid);

	REG_FUNCTION(GetPlayerArmor);
	REG_FUNCTION(IsPlayerAlive);
	REG_FUNCTION(GetEntityClassname);
	REG_FUNCTION(GetEntityModel);

}

LuaHandle* g_LuaHandle = NULL;
LuaHandle* GetLuaHandle() {
	return g_LuaHandle;
}

MainLuaHandle::MainLuaHandle() : LuaHandle() {
	g_LuaHandle = this;
	Register();
}
