#include "cbase.h"
#include "MainLuaHandle.h"
#include "filesystem.h"
#include "convar.h"
#include <stdio.h>
#include "tier0/memdbgon.h"

#include "hl2_player.h"
#include "hl2mp_player.h"

void CC_ReloadLua(const CCommand& args) {
	if (auto handle = GetLuaHandle()) {
		handle->Init();
	}
	else {
		ConMsg("Lua handle not available.\n");
	}
}
static ConCommand reload_lua("reload_lua", CC_ReloadLua, "Reload Lua scripts.", FCVAR_NONE);

void MainLuaHandle::Init() {
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

// server stuff
LUA_FUNC(luaMsg, [](lua_State *L) { Msg("%s\n", lua_tostring(L, 1)); return 0; })
LUA_FUNC(luaConMsg, [](lua_State *L) { ConMsg("%s\n", lua_tostring(L, 1)); return 0; })
LUA_FUNC(luaGetTime, [](lua_State *L) { lua_pushnumber(L, gpGlobals->curtime); return 1; })
LUA_FUNC(luaGetCVar, [](lua_State *L) { lua_pushstring(L, cvar->FindVar(lua_tostring(L, 1))->GetString()); return 1; })
LUA_FUNC(luaSetCVar, [](lua_State *L) { cvar->FindVar(lua_tostring(L, 1))->SetValue(lua_tostring(L, 2)); return 0; })
LUA_FUNC(luaExecuteConsoleCommand, [](lua_State *L) { engine->ServerCommand(lua_tostring(L, 1)); engine->ServerExecute(); return 0; })
LUA_FUNC(luaSay, [](lua_State *L) { std::string msg; for (int i = 1, n = lua_gettop(L); i <= n; ++i) msg += lua_tostring(L, i) + std::string(i < n ? " " : ""); UTIL_ClientPrintAll(HUD_PRINTTALK, msg.c_str()); return 0; })
LUA_FUNC(luaLoadFile, [](lua_State *L) { const char* path = lua_tostring(L, 1); if (!path) { Warning("[LUA-ERR] Invalid file path.\n"); return 0; } FileHandle_t f = filesystem->Open(path, "rb", "MOD"); if (!f) { Warning("[LUA-ERR] Failed to open %s\n", path); return 0; } int size = filesystem->Size(f); char* buffer = (char*)((IFileSystem*)filesystem)->AllocOptimalReadBuffer(f, size + 1); Assert(buffer); ((IFileSystem*)filesystem)->ReadEx(buffer, size + 1, size, f); buffer[size] = '\0'; filesystem->Close(f); if (luaL_loadbuffer(L, buffer, size, path)) { Warning("[LUA-ERR] %s\n", lua_tostring(L, -1)); lua_pop(L, 1); ((IFileSystem*)filesystem)->FreeOptimalReadBuffer(buffer); return 0; } lua_pcall(L, 0, LUA_MULTRET, 0); ((IFileSystem*)filesystem)->FreeOptimalReadBuffer(buffer); return 0; })

// player stuff
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

// entity stuff
LUA_FUNC(luaGetEntityPosition, [](lua_State *L) { auto p = UTIL_EntityByIndex(lua_tointeger(L, 1)); if (p) { auto pos = p->GetAbsOrigin(); lua_pushnumber(L, pos.x); lua_pushnumber(L, pos.y); lua_pushnumber(L, pos.z); } else { lua_pushnil(L); lua_pushnil(L); lua_pushnil(L); } return 3; })
LUA_FUNC(luaGetEntityClassname, [](lua_State *L) { auto p = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushstring(L, p ? p->GetClassname() : nullptr); return 1; })
LUA_FUNC(luaSpawnEntity, [](lua_State *L) { const char* classname = lua_tostring(L, 1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float z = lua_tonumber(L, 4);
	auto ent = (CBaseEntity*)CreateEntityByName(classname);
	if (ent) {
		ent->SetAbsOrigin(Vector(x, y, z));
		DispatchSpawn(ent);
	}
	return 0;
})
LUA_FUNC(luaTeleportPlayer, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) {
		float x = lua_tonumber(L, 2);
		float y = lua_tonumber(L, 3);
		float z = lua_tonumber(L, 4);
		p->Teleport(&Vector(x, y, z), nullptr, nullptr);
	}
	return 0;
})
LUA_FUNC(luaSetEntityProperty, [](lua_State *L) {
	auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1));
	const char* prop = lua_tostring(L, 2);
	if (ent && prop) {
		if (strcmp(prop, "health") == 0) {
			ent->SetHealth(lua_tointeger(L, 3));
		}
		else if (strcmp(prop, "position") == 0) {
			float x = lua_tonumber(L, 3);
			float y = lua_tonumber(L, 4);
			float z = lua_tonumber(L, 5);
			ent->SetAbsOrigin(Vector(x, y, z));
		}
		else if (strcmp(prop, "velocity") == 0) {
			float vx = lua_tonumber(L, 3);
			float vy = lua_tonumber(L, 4);
			float vz = lua_tonumber(L, 5);
			ent->SetAbsVelocity(Vector(vx, vy, vz));
		}
		else if (strcmp(prop, "name") == 0) {
			ent->SetName(AllocPooledString(lua_tostring(L, 3)));
		}
		else if (strcmp(prop, "angles") == 0) {
			float pitch = lua_tonumber(L, 3);
			float yaw = lua_tonumber(L, 4);
			float roll = lua_tonumber(L, 5);
			QAngle angles(pitch, yaw, roll);
			ent->SetAbsAngles(angles);
		}
		else if (strcmp(prop, "model") == 0) {
			const char* modelName = lua_tostring(L, 3);
			ent->SetModel(modelName);
		}
		else if (strcmp(prop, "solid") == 0) {
			bool solid = lua_toboolean(L, 3);
			ent->SetSolid(solid ? SOLID_VPHYSICS : SOLID_NONE);
		}
		else if (strcmp(prop, "color") == 0) {
			int r = lua_tointeger(L, 3);
			int b = lua_tointeger(L, 4);
			int g = lua_tointeger(L, 5);
			int a = lua_tointeger(L, 6);
			ent->SetRenderColor(r, g, b, a);
		}
	}
	return 0;
})
LUA_FUNC(luaIsPlayerNearEntity, [](lua_State *L) { auto player = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	auto ent = UTIL_EntityByIndex(lua_tointeger(L, 2));
	float radius = lua_tonumber(L, 3);
	if (player && ent) {
		float distance = (player->GetAbsOrigin() - ent->GetAbsOrigin()).Length();
		lua_pushboolean(L, distance <= radius);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(luaGetEntityVelocity, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); if (ent) { const auto& velocity = ent->GetAbsVelocity(); lua_pushnumber(L, velocity.x); lua_pushnumber(L, velocity.y); lua_pushnumber(L, velocity.z); } else { lua_pushnil(L); lua_pushnil(L); lua_pushnil(L); } return 3; })
LUA_FUNC(luaKillEntity, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); if (ent) UTIL_Remove(ent); return 0; })
LUA_FUNC(luaSetPlayerMaxSpeed, [](lua_State *L) { auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1)); if (p) p->SetMaxSpeed(lua_tonumber(L, 2)); return 0; })
LUA_FUNC(luaGetEntityName, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushstring(L, ent ? ent->GetEntityName().ToCStr() : nullptr); return 1; })
LUA_FUNC(luaGetEntityModel, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushstring(L, ent ? ent->GetModelName().ToCStr() : nullptr); return 1; })
LUA_FUNC(luaGetEntityClassId, [](lua_State *L) { auto ent = UTIL_EntityByIndex(lua_tointeger(L, 1)); lua_pushinteger(L, ent ? ent->Classify() : -1); return 1; })

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
	REG_FUNCTION(LoadFile);
	REG_FUNCTION(KillPlayer);
	REG_FUNCTION(SpawnEntity);
	REG_FUNCTION(TeleportPlayer);
	REG_FUNCTION(SetEntityProperty);
	REG_FUNCTION(IsPlayerNearEntity);
}

LuaHandle* g_LuaHandle = NULL;
LuaHandle* GetLuaHandle() {
	return g_LuaHandle;
}

MainLuaHandle::MainLuaHandle() : LuaHandle() {
	g_LuaHandle = this;
	Register();
}
