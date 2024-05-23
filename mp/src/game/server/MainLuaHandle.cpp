#include "cbase.h"
#include "MainLuaHandle.h"
#include "filesystem.h"
#include "convar.h"
#include <stdio.h>
#include "tier0/memdbgon.h"

#include "hl2_player.h"
#include "hl2mp_player.h"
#include "player.h"
#include "usermessages.h"

void CC_ReloadLua(const CCommand& args) {
	if (auto handle = GetLuaHandle()) {
		handle->Init();
	}
	else {
		ConMsg("Lua handle not available.\n");
	}
}
static ConCommand reload_lua("reload_lua", CC_ReloadLua, "Reload Lua scripts.", FCVAR_NONE);

void SandboxLua(lua_State* L) {
	lua_newtable(L);
	lua_newtable(L); 

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	lua_setmetatable(L, -2);

	lua_replace(L, -2);

	luaL_dostring(L, "os = nil");
	luaL_dostring(L, "io = nil");
	luaL_dostring(L, "require = nil");
}

void PlayerJoinGame(int playerIndex, lua_State* L) {
	lua_getglobal(L, "OnPlayerJoin");
	if (lua_isfunction(L, -1)) {
		lua_pushinteger(L, playerIndex);
		lua_pcall(L, 1, 0, 0);
	}
	else {
		lua_pop(L, 1);
	}
}

void PlayerLeaveGame(int playerIndex, lua_State* L) {
	lua_getglobal(L, "OnPlayerLeave");
	if (lua_isfunction(L, -1)) {
		lua_pushinteger(L, playerIndex);
		lua_pcall(L, 1, 0, 0);
	}
	else {
		lua_pop(L, 1);
	}
}

void MainLuaHandle::Init() {
	SandboxLua(GetLua());
	const char* fullPath = "lua/main.lua";
	FileHandle_t f = filesystem->Open(fullPath, "rb", "MOD");
	if (!f) {
		Warning("[LUA-ERR] Failed to open %s\n", fullPath);
		return;
	}

	int fileSize = filesystem->Size(f);
	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, fileSize + 1);
	Assert(buffer);

	((IFileSystem *)filesystem)->ReadEx(buffer, fileSize + 1, fileSize, f);
	buffer[fileSize] = '\0';
	filesystem->Close(f);

	if (luaL_loadbuffer(GetLua(), buffer, fileSize, fullPath)) {
		Warning("[LUA-ERR] %s\n", lua_tostring(GetLua(), -1));
		lua_pop(GetLua(), 1);
		((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
		return;
	}

	CallLUA(GetLua(), 0, LUA_MULTRET, 0, fullPath);
	m_bLuaLoaded = true;
	((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
}

void LoadLua(const char* filePath, const bool isEvented) {
	if (!filePath) {
		Warning("[LUA-ERR] Invalid file path.\n");
		return;
	}

	FileHandle_t f = filesystem->Open(filePath, "rb", "MOD");
	if (!f) {
		Warning("[LUA-ERR] Failed to open %s\n", filePath);
		return;
	}

	int size = filesystem->Size(f);
	char* buffer = (char*)((IFileSystem*)filesystem)->AllocOptimalReadBuffer(f, size + 1);
	Assert(buffer);
	((IFileSystem*)filesystem)->ReadEx(buffer, size + 1, size, f);
	buffer[size] = '\0';
	filesystem->Close(f);

	if (luaL_loadbuffer(GetLuaHandle()->GetLua(), buffer, size, filePath)) {
		Warning("[LUA-ERR] %s\n", lua_tostring(GetLuaHandle()->GetLua(), -1));
		lua_pop(GetLuaHandle()->GetLua(), 1);
		((IFileSystem*)filesystem)->FreeOptimalReadBuffer(buffer);
		return;
	}

	if (isEvented) {
		PlayerJoinGame(1, GetLuaHandle()->GetLua());
		PlayerLeaveGame(1, GetLuaHandle()->GetLua());
	}
	lua_pcall(GetLuaHandle()->GetLua(), 0, LUA_MULTRET, 0);
	((IFileSystem*)filesystem)->FreeOptimalReadBuffer(buffer);
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

#define LUA_FUNC(name, func) int name(lua_State *L) { return func(L); }

#pragma warning(disable: 4238)
#pragma warning(disable: 4800)
#pragma warning(disable: 4189)

LUA_FUNC(luaMsg, [](lua_State *L) { Msg("%s\n", lua_tostring(L, 1)); return 0; })
LUA_FUNC(luaConMsg, [](lua_State *L) { ConMsg("%s\n", lua_tostring(L, 1)); return 0; })
LUA_FUNC(luaGetTime, [](lua_State *L) { lua_pushnumber(L, gpGlobals->curtime); return 1; })
LUA_FUNC(luaGetCVar, [](lua_State *L) { lua_pushstring(L, cvar->FindVar(lua_tostring(L, 1))->GetString()); return 1; })
LUA_FUNC(luaSetCVar, [](lua_State *L) { cvar->FindVar(lua_tostring(L, 1))->SetValue(lua_tostring(L, 2)); return 0; })
LUA_FUNC(luaExecuteConsoleCommand, [](lua_State *L) { engine->ServerCommand(lua_tostring(L, 1)); engine->ServerExecute(); return 0; })
LUA_FUNC(luaSay, [](lua_State *L) { std::string msg; for (int i = 1, n = lua_gettop(L); i <= n; ++i) msg += lua_tostring(L, i) + std::string(i < n ? " " : ""); UTIL_ClientPrintAll(HUD_PRINTTALK, msg.c_str()); return 0; })
LUA_FUNC(luaLoadFile, [](lua_State *L) { const char* path = lua_tostring(L, 1); if (!path) { Warning("[LUA-ERR] Invalid file path.\n"); return 0; } FileHandle_t f = filesystem->Open(path, "rb", "MOD"); if (!f) { Warning("[LUA-ERR] Failed to open %s\n", path); return 0; } int size = filesystem->Size(f); char* buffer = (char*)((IFileSystem*)filesystem)->AllocOptimalReadBuffer(f, size + 1); Assert(buffer); ((IFileSystem*)filesystem)->ReadEx(buffer, size + 1, size, f); buffer[size] = '\0'; filesystem->Close(f); if (luaL_loadbuffer(L, buffer, size, path)) { Warning("[LUA-ERR] %s\n", lua_tostring(L, -1)); lua_pop(L, 1); ((IFileSystem*)filesystem)->FreeOptimalReadBuffer(buffer); return 0; } lua_pcall(L, 0, LUA_MULTRET, 0); ((IFileSystem*)filesystem)->FreeOptimalReadBuffer(buffer); return 0; })
LUA_FUNC(luaCreateCVar, [](lua_State* L) {	const char* name = lua_tostring(L, 1);	const char* defaultValue = lua_tostring(L, 2);	ConVar cvar(name, defaultValue);	return 0; })
LUA_FUNC(luaCreateConCommand, [](lua_State* L) { const char* name = lua_tostring(L, 1); const char* description = lua_tostring(L, 2); FnCommandCallbackVoid_t callback = (FnCommandCallbackVoid_t)lua_touserdata(L, 3); if (callback == nullptr) { lua_pushstring(L, "Invalid function pointer provided"); lua_error(L); return 0; } ConCommand concommand(name, callback, description); return 0;})
LUA_FUNC(luaRegisterFunction, [](lua_State* L) {	const char* functionName = lua_tostring(L, 1);	void* functionPtr = lua_touserdata(L, 2);	if (functionPtr == nullptr) { lua_pushstring(L, "Invalid function pointer provided"); lua_error(L); return 0; }	lua_register(L, functionName, (lua_CFunction)functionPtr);	return 0; })
LUA_FUNC(luaSetGlobalVariable, [](lua_State* L) {	const char* variableName = lua_tostring(L, 1);	lua_setglobal(L, variableName);	return 0; })
LUA_FUNC(luaGetPlayerName, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); lua_pushstring(L, p ? p->GetPlayerName() : nullptr); return 1; })
LUA_FUNC(luaGetPlayerHealth, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); lua_pushinteger(L, p ? p->GetHealth() : -1); return 1; })
LUA_FUNC(luaGetPlayerPosition, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p) { auto pos = p->GetAbsOrigin(); lua_pushnumber(L, pos.x); lua_pushnumber(L, pos.y); lua_pushnumber(L, pos.z); } else { lua_pushnil(L); lua_pushnil(L); lua_pushnil(L); } return 3; })
LUA_FUNC(luaHandlePlayerInput, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p && strcmp(lua_tostring(L, 2), "jump") == 0) p->Jump(); lua_pushboolean(L, p != nullptr); return 1; })
LUA_FUNC(luaGetPlayerCount, [](lua_State *L) { lua_pushinteger(L, gpGlobals->maxClients); return 1; })
LUA_FUNC(luaGetPlayerTeam, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); lua_pushinteger(L, p ? p->GetTeamNumber() : -1); return 1; })
LUA_FUNC(luaGetPlayerMaxSpeed, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); lua_pushinteger(L, p ? p->GetPlayerMaxSpeed() : -1); return 1; })
LUA_FUNC(luaGiveItem, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p) p->GiveNamedItem(lua_tostring(L, 2)); return 0; })
LUA_FUNC(luaIsPlayerValid, [](lua_State *L) { lua_pushboolean(L, UTIL_PlayerByIndex(lua_tointeger(L, 1)) != nullptr); return 1; })
LUA_FUNC(luaGetPlayerArmor, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); lua_pushinteger(L, p ? p->ArmorValue() : -1); return 1; })
LUA_FUNC(luaIsPlayerAlive, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); lua_pushboolean(L, p && p->IsAlive()); return 1; })
LUA_FUNC(luaKillPlayer, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p) p->TakeDamage(CTakeDamageInfo(p, p, p->GetHealth() + p->ArmorValue(), DMG_GENERIC)); return 0; })
LUA_FUNC(luaSetPlayerArmor, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p) p->SetArmorValue(lua_tointeger(L, 2)); return 0; })
LUA_FUNC(luaGetEntityPosition, [](lua_State *L) { auto p = UTIL_EntityByIndex(lua_tointeger(L, 1)); if (p) { auto pos = p->GetAbsOrigin(); lua_pushnumber(L, pos.x); lua_pushnumber(L, pos.y); lua_pushnumber(L, pos.z); } else { lua_pushnil(L); lua_pushnil(L); lua_pushnil(L); } return 3; })
LUA_FUNC(luaGetEntityClassname, [](lua_State *L) { auto p = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushstring(L, p ? p->GetClassname() : nullptr); return 1; })
LUA_FUNC(luaSpawnEntity, [](lua_State *L) { const char* classname = lua_tostring(L, 1);	float x = lua_tonumber(L, 2);	float y = lua_tonumber(L, 3);	float z = lua_tonumber(L, 4);	auto ent = (CBaseEntity*)CreateEntityByName(classname);	if (ent) { ent->SetAbsOrigin(Vector(x, y, z)); DispatchSpawn(ent); }	return 0; })
LUA_FUNC(luaTeleportPlayer, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));	if (p) { float x = lua_tonumber(L, 2); float y = lua_tonumber(L, 3); float z = lua_tonumber(L, 4); p->Teleport(&Vector(x, y, z), nullptr, nullptr); }	return 0; })
LUA_FUNC(luaSetEntityProperty, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); const char* prop = lua_tostring(L, 2); if (ent && prop) { if (strcmp(prop, "health") == 0) { ent->SetHealth(lua_tointeger(L, 3)); } else if (strcmp(prop, "position") == 0) { float x = lua_tonumber(L, 3); float y = lua_tonumber(L, 4); float z = lua_tonumber(L, 5); ent->SetAbsOrigin(Vector(x, y, z)); } else if (strcmp(prop, "velocity") == 0) { float vx = lua_tonumber(L, 3); float vy = lua_tonumber(L, 4); float vz = lua_tonumber(L, 5); ent->SetAbsVelocity(Vector(vx, vy, vz)); } else if (strcmp(prop, "name") == 0) { ent->SetName(AllocPooledString(lua_tostring(L, 3))); } else if (strcmp(prop, "angles") == 0) { float pitch = lua_tonumber(L, 3); float yaw = lua_tonumber(L, 4); float roll = lua_tonumber(L, 5); QAngle angles(pitch, yaw, roll); ent->SetAbsAngles(angles); } else if (strcmp(prop, "model") == 0) { const char* modelName = lua_tostring(L, 3); ent->SetModel(modelName); } else if (strcmp(prop, "solid") == 0) { bool solid = lua_toboolean(L, 3); ent->SetSolid(solid ? SOLID_VPHYSICS : SOLID_NONE); } else if (strcmp(prop, "color") == 0) { int r = lua_tointeger(L, 3); int b = lua_tointeger(L, 4); int g = lua_tointeger(L, 5); int a = lua_tointeger(L, 6); ent->SetRenderColor(r, g, b, a); } } return 0;})LUA_FUNC(luaIsPlayerNearEntity, [](lua_State *L) { auto player = UTIL_PlayerByIndex(lua_tointeger(L, 1));	auto ent = UTIL_EntityByIndex(lua_tointeger(L, 2));	float radius = lua_tonumber(L, 3);	if (player && ent) { float distance = (player->GetAbsOrigin() - ent->GetAbsOrigin()).Length(); lua_pushboolean(L, distance <= radius); } else { lua_pushboolean(L, false); }	return 1; })
LUA_FUNC(luaGetEntityVelocity, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); if (ent) { const auto& velocity = ent->GetAbsVelocity(); lua_pushnumber(L, velocity.x); lua_pushnumber(L, velocity.y); lua_pushnumber(L, velocity.z); } else { lua_pushnil(L); lua_pushnil(L); lua_pushnil(L); } return 3; })
LUA_FUNC(luaKillEntity, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); if (ent) UTIL_Remove(ent); return 0; })
LUA_FUNC(luaSetPlayerMaxSpeed, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p) p->SetMaxSpeed(lua_tonumber(L, 2)); return 0; })
LUA_FUNC(luaGetEntityName, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushstring(L, ent ? ent->GetEntityName().ToCStr() : nullptr); return 1; })
LUA_FUNC(luaGetEntityModel, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushstring(L, ent ? ent->GetModelName().ToCStr() : nullptr); return 1; })
LUA_FUNC(luaGiveAmmo, [](lua_State *L) { int playerIndex = lua_tointeger(L, 1); const char* ammoType = lua_tostring(L, 2); int amount = lua_tointeger(L, 3); if (!UTIL_PlayerByIndex(playerIndex)) { lua_pushstring(L, "Invalid player index"); return 1; } UTIL_PlayerByIndex(playerIndex)->GiveAmmo(amount, ammoType); return 0; })
LUA_FUNC(luaSetPlayerHealth, [](lua_State *L) { int playerIndex = lua_tointeger(L, 1); int health = lua_tointeger(L, 2); auto player = UTIL_PlayerByIndex(playerIndex); if (!player) { lua_pushstring(L, "Invalid player index"); return 1; } player->SetHealth(health); return 0;})
int g_LastPlayerIndex = 1; void SetLastPlayerIndex(int playerIndex) { g_LastPlayerIndex = playerIndex; }LUA_FUNC(luaGetNewPlayer, [](lua_State *L) {	lua_pushinteger(L, g_LastPlayerIndex);	return 1; });
LUA_FUNC(luaAddNumbers, [](lua_State *L) { double num1 = lua_tonumber(L, 1); double num2 = lua_tonumber(L, 2); lua_pushnumber(L, num1 + num2); return 1; })
LUA_FUNC(luaSubtractNumbers, [](lua_State *L) { double num1 = lua_tonumber(L, 1); double num2 = lua_tonumber(L, 2); lua_pushnumber(L, num1 - num2); return 1; })
LUA_FUNC(luaMultiplyNumbers, [](lua_State *L) { double num1 = lua_tonumber(L, 1); double num2 = lua_tonumber(L, 2); lua_pushnumber(L, num1 * num2); return 1; })
LUA_FUNC(luaDivideNumbers, [](lua_State *L) { double num1 = lua_tonumber(L, 1); double num2 = lua_tonumber(L, 2); if (num2 == 0) { lua_pushstring(L, "Error: Division by zero"); return 1; } lua_pushnumber(L, num1 / num2); return 1; })
LUA_FUNC(luaIsNumberPositive, [](lua_State *L) { double num = lua_tonumber(L, 1); lua_pushboolean(L, num > 0); return 1; })
LUA_FUNC(luaIsNumberNegative, [](lua_State *L) { double num = lua_tonumber(L, 1); lua_pushboolean(L, num < 0); return 1; })
LUA_FUNC(luaIsNumberZero, [](lua_State *L) { double num = lua_tonumber(L, 1); lua_pushboolean(L, num == 0); return 1; })
LUA_FUNC(luaIsStringEmpty, [](lua_State *L) { const char* str = lua_tostring(L, 1); lua_pushboolean(L, str == nullptr || str[0] == '\0'); return 1; })
LUA_FUNC(luaStringLength, [](lua_State *L) { const char* str = lua_tostring(L, 1); lua_pushinteger(L, str ? strlen(str) : 0); return 1; })
LUA_FUNC(luaConcatStrings, [](lua_State *L) { std::string result; for (int i = 1, n = lua_gettop(L); i <= n; ++i) { const char* str = lua_tostring(L, i); if (str) result += str; } lua_pushstring(L, result.c_str()); return 1; })
LUA_FUNC(luaStringCompare, [](lua_State *L) { const char* str1 = lua_tostring(L, 1); const char* str2 = lua_tostring(L, 2); lua_pushboolean(L, strcmp(str1, str2) == 0); return 1; })
LUA_FUNC(luaStringSubstring, [](lua_State *L) { const char* str = lua_tostring(L, 1); int start = lua_tointeger(L, 2); int length = lua_tointeger(L, 3); if (!str) { lua_pushstring(L, ""); return 1; } std::string result = std::string(str).substr(start, length); lua_pushstring(L, result.c_str()); return 1; })
LUA_FUNC(luaStringFind, [](lua_State *L) { const char* str = lua_tostring(L, 1); const char* pattern = lua_tostring(L, 2); if (!str || !pattern) { lua_pushinteger(L, -1); return 1; } const char* found = strstr(str, pattern); if (found) lua_pushinteger(L, found - str); else lua_pushinteger(L, -1); return 1; })
LUA_FUNC(luaStringReplace, [](lua_State *L) { const char* str = lua_tostring(L, 1); const char* target = lua_tostring(L, 2); const char* replacement = lua_tostring(L, 3); if (!str || !target || !replacement) { lua_pushstring(L, ""); return 1; } std::string result(str); size_t pos = result.find(target); while (pos != std::string::npos) { result.replace(pos, strlen(target), replacement); pos = result.find(target, pos + strlen(replacement)); } lua_pushstring(L, result.c_str()); return 1; })
LUA_FUNC(luaTableHasKey, [](lua_State *L) { lua_pushboolean(L, lua_istable(L, 1) && lua_getfield(L, 1, lua_tostring(L, 2)) != LUA_TNIL); return 1; })
LUA_FUNC(luaTableMerge, [](lua_State *L) { if (!lua_istable(L, 1) || !lua_istable(L, 2)) return 0; lua_pushnil(L); while (lua_next(L, 2) != 0) { lua_pushvalue(L, -2); lua_pushvalue(L, -2); lua_rawset(L, 1); lua_pop(L, 1); } return 0; })
LUA_FUNC(luaTableClear, [](lua_State *L) { lua_pushnil(L); while (lua_next(L, 1) != 0) { lua_pop(L, 1); lua_pushvalue(L, -1); lua_pushnil(L); lua_rawset(L, 1); } return 0; })
LUA_FUNC(luaTableCopy, [](lua_State *L) { if (!lua_istable(L, 1)) return 0; lua_newtable(L); lua_pushnil(L); while (lua_next(L, 1) != 0) { lua_pushvalue(L, -2); lua_pushvalue(L, -2); lua_rawset(L, -5); lua_pop(L, 1); } return 1; })
LUA_FUNC(luaTableRemoveAt, [](lua_State *L) { int index = lua_tointeger(L, 2); if (index <= 0) return 0; lua_pushnil(L); lua_rawseti(L, 1, index); return 0; })
LUA_FUNC(luaTableForEach, [](lua_State *L) { if (!lua_isfunction(L, 2) || !lua_istable(L, 1)) return 0; lua_pushnil(L); while (lua_next(L, 1) != 0) { lua_pushvalue(L, 2); lua_pushvalue(L, -3); lua_pushvalue(L, -3); lua_call(L, 2, 0); lua_pop(L, 1); } return 0; })
LUA_FUNC(luaTableSort, [](lua_State *L) { if (!lua_isfunction(L, 2) || !lua_istable(L, 1)) return 0; lua_pushvalue(L, 2); lua_pushvalue(L, 1); lua_call(L, 1, 0); return 0; })
LUA_FUNC(luaTableSlice, [](lua_State *L) { int start = lua_tointeger(L, 2); int length = lua_tointeger(L, 3); if (start < 1 || length < 0) return 0; lua_newtable(L); int tableLength = luaL_len(L, 1); for (int i = start; i < start + length && i <= tableLength; ++i) { lua_rawgeti(L, 1, i); lua_rawseti(L, -2, i - start + 1); } return 1; })
LUA_FUNC(luaTablePush, [](lua_State *L) { int n = lua_gettop(L); for (int i = 2; i <= n; ++i) { lua_pushvalue(L, i); lua_rawseti(L, 1, luaL_len(L, 1) + 1); } return 0; })
LUA_FUNC(luaTablePop, [](lua_State *L) { lua_pushnil(L); lua_rawseti(L, 1, luaL_len(L, 1)); return 0; })
LUA_FUNC(luaTableConcat, [](lua_State *L) { if (!lua_istable(L, 1)) return 0; std::string result; lua_pushnil(L); while (lua_next(L, 1) != 0) { if (lua_isstring(L, -1)) result += lua_tostring(L, -1); lua_pop(L, 1); } lua_pushstring(L, result.c_str()); return 1; })
LUA_FUNC(luaGetEntityByClassname, [](lua_State *L) { const char* classname = lua_tostring(L, 1); CBaseEntity* ent = nullptr; while ((ent = gEntList.FindEntityByClassname(ent, classname)) != nullptr) { if (!ent->IsMarkedForDeletion()) { lua_pushinteger(L, ent->entindex()); return 1; } } lua_pushnil(L); return 1;})LUA_FUNC(luaTraceLine, [](lua_State *L) { Vector start(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3)); Vector end(lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6)); Ray_t ray; ray.Init(start, end); CTraceFilterWorldAndPropsOnly filter; trace_t tr; enginetrace->TraceRay(ray, MASK_ALL, &filter, &tr); if (tr.fraction < 1.0f) { lua_pushnumber(L, tr.endpos.x); lua_pushnumber(L, tr.endpos.y); lua_pushnumber(L, tr.endpos.z); return 3; } lua_pushnil(L); lua_pushnil(L); lua_pushnil(L); return 3;})LUA_FUNC(luaCreateEntity, [](lua_State *L) { const char* classname = lua_tostring(L, 1); if (!classname) { lua_pushnil(L); return 1; } CBaseEntity* ent = CreateEntityByName(classname); if (ent) { DispatchSpawn(ent); lua_pushinteger(L, ent->entindex()); return 1; } lua_pushnil(L); return 1;})LUA_FUNC(luaSetEntityModel, [](lua_State *L) { CBaseEntity* ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); const char* modelName = lua_tostring(L, 2); if (ent && modelName) { ent->SetModel(modelName); } return 0;})LUA_FUNC(luaSetEntityAngles, [](lua_State *L) { CBaseEntity* ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); float pitch = lua_tonumber(L, 2); float yaw = lua_tonumber(L, 3); float roll = lua_tonumber(L, 4); if (ent) { QAngle angles(pitch, yaw, roll); ent->SetAbsAngles(angles); } return 0;})
LUA_FUNC(luaSayToClient, [](lua_State *L) {
	int clientIndex = lua_tointeger(L, 1);
	std::string msg;
	for (int i = 2, n = lua_gettop(L); i <= n; ++i)
		msg += lua_tostring(L, i) + std::string(i < n ? " " : "");
	CBasePlayer* player = UTIL_PlayerByIndex(clientIndex);
	if (player)
		ClientPrint(player, HUD_PRINTTALK, msg.c_str());
	return 0;
})

void MainLuaHandle::RegFunctions() {
	REG_FUNCTION(Msg);
	REG_FUNCTION(ConMsg);
	REG_FUNCTION(GetTime);
	REG_FUNCTION(GetCVar);
	REG_FUNCTION(SetCVar);
	REG_FUNCTION(ExecuteConsoleCommand);
	REG_FUNCTION(Say);
	REG_FUNCTION(LoadFile);
	REG_FUNCTION(CreateCVar);
	REG_FUNCTION(CreateConCommand);
	REG_FUNCTION(RegisterFunction);
	REG_FUNCTION(SetGlobalVariable);
	REG_FUNCTION(GetPlayerName);
	REG_FUNCTION(GetPlayerHealth);
	REG_FUNCTION(GetPlayerPosition);
	REG_FUNCTION(HandlePlayerInput);
	REG_FUNCTION(GetPlayerCount);
	REG_FUNCTION(GetPlayerTeam);
	REG_FUNCTION(GetPlayerMaxSpeed);
	REG_FUNCTION(GiveItem);
	REG_FUNCTION(IsPlayerValid);
	REG_FUNCTION(GetPlayerArmor);
	REG_FUNCTION(IsPlayerAlive);
	REG_FUNCTION(KillPlayer);
	REG_FUNCTION(SetPlayerArmor);
	REG_FUNCTION(GetEntityPosition);
	REG_FUNCTION(GetEntityClassname);
	REG_FUNCTION(SpawnEntity);
	REG_FUNCTION(TeleportPlayer);
	REG_FUNCTION(SetEntityProperty);
	REG_FUNCTION(IsPlayerNearEntity);
	REG_FUNCTION(GetEntityVelocity);
	REG_FUNCTION(KillEntity);
	REG_FUNCTION(SetPlayerMaxSpeed);
	REG_FUNCTION(GetEntityName);
	REG_FUNCTION(GetEntityModel);
	REG_FUNCTION(GiveAmmo);
	REG_FUNCTION(SetPlayerHealth);
	REG_FUNCTION(GetNewPlayer);
	REG_FUNCTION(AddNumbers);
	REG_FUNCTION(SubtractNumbers);
	REG_FUNCTION(MultiplyNumbers);
	REG_FUNCTION(DivideNumbers);
	REG_FUNCTION(IsNumberPositive);
	REG_FUNCTION(IsNumberNegative);
	REG_FUNCTION(IsNumberZero);
	REG_FUNCTION(IsStringEmpty);
	REG_FUNCTION(StringLength);
	REG_FUNCTION(ConcatStrings);
	REG_FUNCTION(StringCompare);
	REG_FUNCTION(StringSubstring);
	REG_FUNCTION(StringFind);
	REG_FUNCTION(StringReplace);
	REG_FUNCTION(TableHasKey);
	REG_FUNCTION(TableMerge);
	REG_FUNCTION(TableClear);
	REG_FUNCTION(TableCopy);
	REG_FUNCTION(TableRemoveAt);
	REG_FUNCTION(TableForEach);
	REG_FUNCTION(TableSort);
	REG_FUNCTION(TableSlice);
	REG_FUNCTION(TablePush);
	REG_FUNCTION(TablePop);
	REG_FUNCTION(TableConcat);
	REG_FUNCTION(GetEntityByClassname);
	REG_FUNCTION(TraceLine);
	REG_FUNCTION(CreateEntity);
	REG_FUNCTION(SetEntityModel);
	REG_FUNCTION(SetEntityAngles);
	REG_FUNCTION(SayToClient);
}

LuaHandle* g_LuaHandle = NULL;
LuaHandle* GetLuaHandle() {
	return g_LuaHandle;
}

MainLuaHandle::MainLuaHandle() : LuaHandle() {
	g_LuaHandle = this;
	Register();
}
