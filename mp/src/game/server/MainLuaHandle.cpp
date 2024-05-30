#include "cbase.h"
#include "MainLuaHandle.h"
#include "filesystem.h"
#include "convar.h"
#include <stdio.h>
#include "tier0/memdbgon.h"
#include "engine/IEngineSound.h"
#include <vector>
#include <string>
#include "hl2_player.h"
#include "hl2mp_player.h"
#include "player.h"
#include "usermessages.h"
#include "engine\iserverplugin.h"
#include <cstdlib>
#include <iostream>


void CC_ReloadLua(const CCommand& args) {
	if (auto handle = GetLuaHandle()) {
		handle->Init();
	}
	else {
		Warning("Lua handle not available.\n");
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

void GetFilesInDirectory(const char* directory, const char* fileExtension, std::vector<std::string>& outFiles) {
	char searchPath[MAX_PATH];
	Q_snprintf(searchPath, sizeof(searchPath), "%s/*", directory);

	FileFindHandle_t findHandle;
	const char* fileName = filesystem->FindFirst(searchPath, &findHandle);
	while (fileName) {
		if (Q_strcmp(fileName, ".") != 0 && Q_strcmp(fileName, "..") != 0) {
			char filePath[MAX_PATH];
			Q_snprintf(filePath, sizeof(filePath), "%s/%s", directory, fileName);
			if (filesystem->IsDirectory(filePath)) {
				GetFilesInDirectory(filePath, fileExtension, outFiles);
			}
			else {
				if (Q_stristr(fileName, fileExtension)) {
					outFiles.push_back(filePath);
				}
			}
		}
		fileName = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
}

void LoadLuaFiles(const char* directory) {
	std::vector<std::string> luaFiles;
	GetFilesInDirectory(directory, ".lua", luaFiles);

	for (const auto& luaFile : luaFiles) {
		LoadLua(luaFile.c_str());
	}
}

void PrecacheResources(const char* directory, const char* fileExtension, void(*precacheFunc)(const char*)) {
	std::vector<std::string> resourceFiles;
	GetFilesInDirectory(directory, fileExtension, resourceFiles);

	for (const auto& resourceFile : resourceFiles) {
		precacheFunc(resourceFile.c_str());
	}
}

void PrecacheModel(const char* model) {
	CBaseEntity::PrecacheModel(model);
}

void PrecacheSound(const char* sound) {
	CBaseEntity::PrecacheSound(sound);
}


void LoadAddons() {
	const char* addonsDirectory = "addons/";

	if (!filesystem->IsDirectory(addonsDirectory)) {
		Error("[LUA-ERR] addons directory not found\n");
		return;
	}

	char searchPath[MAX_PATH];
	Q_snprintf(searchPath, sizeof(searchPath), "%s/*", addonsDirectory);

	FileFindHandle_t findHandle;
	const char* addonName = filesystem->FindFirst(searchPath, &findHandle);
	while (addonName) {
		if (Q_strcmp(addonName, ".") != 0 && Q_strcmp(addonName, "..") != 0) {
			char addonPath[MAX_PATH];
			Q_snprintf(addonPath, sizeof(addonPath), "%s/%s", addonsDirectory, addonName);

			if (filesystem->IsDirectory(addonPath)) {
				const char* subDirs[] = { "models", "materials", "sounds", "lua" };
				for (const char* subDir : subDirs) {
					char subDirPath[MAX_PATH];
					Q_snprintf(subDirPath, sizeof(subDirPath), "%s/%s", addonPath, subDir);

					if (filesystem->IsDirectory(subDirPath)) {
						if (Q_strcmp(subDir, "lua") == 0) {
							char mainLuaPath[MAX_PATH];
							Q_snprintf(mainLuaPath, sizeof(mainLuaPath), "%s/main.lua", subDirPath);
							if (filesystem->FileExists(mainLuaPath)) {
								LoadLua(mainLuaPath);
							}
						}
						else if (Q_strcmp(subDir, "models") == 0) {
							PrecacheResources(subDirPath, ".mdl", PrecacheModel);
						}
						else if (Q_strcmp(subDir, "materials") == 0) {
							PrecacheResources(subDirPath, ".vmt", PrecacheMaterial);
						}
						else if (Q_strcmp(subDir, "sounds") == 0) {
							PrecacheResources(subDirPath, ".wav", PrecacheSound);
						}
					}
				}
			}
		}
		addonName = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);

	Msg("[LUA] addons successfully loaded!\n");
}

void LoadLua(const char* filePath) {
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

// server-related
LUA_FUNC(lua_StartNextLevel, [](lua_State *L) {
	const char* mapName = lua_tostring(L, 1);
	if (mapName) {
		engine->ChangeLevel(mapName, nullptr);
	}
	return 0;
})

LUA_FUNC(lua_GetCurrentMap, [](lua_State *L) {
	const char* currentMap = STRING(gpGlobals->mapname);
	lua_pushstring(L, currentMap);
	return 1;
})

LUA_FUNC(lua_ServerCommand, [](lua_State *L) {
	engine->ServerCommand(lua_tostring(L, 1));
	engine->ServerExecute();
	return 0;
})

LUA_FUNC(lua_Msg, [](lua_State * L) {
	Msg("%s\n", lua_tostring(L, 1));
	return 0;
})

LUA_FUNC(lua_CurTime, [](lua_State * L) {
	lua_pushnumber(L, gpGlobals->curtime);
	return 1;
})

LUA_FUNC(lua_MaxPlayers, [](lua_State * L) {
	lua_pushnumber(L, gpGlobals->maxClients);
	return 1;
})

LUA_FUNC(lua_GetModPath, [](lua_State *L) {
	char modPath[MAX_PATH];
	if (filesystem->GetCurrentDirectory(modPath, sizeof(modPath))) {
		lua_pushstring(L, modPath);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
})

LUA_FUNC(lua_IsDedicatedServer, [](lua_State *L) {
	lua_pushboolean(L, engine->IsDedicatedServer());
	return 1;
})

LUA_FUNC(lua_GetConVar_Float, [](lua_State *L) {
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

LUA_FUNC(lua_GetConVar_String, [](lua_State *L) {
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

LUA_FUNC(lua_GetConVar_Bool, [](lua_State *L) {
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

// player-related
LUA_FUNC(lua_GiveItem, [](lua_State * L) {
	auto p = UTIL_PlayerByIndex(lua_tointeger(L, 1));
	if (p) p->GiveNamedItem(lua_tostring(L, 2));
	return 0;
})
LUA_FUNC(lua_GiveAmmo, [](lua_State * L) {
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

// file manipulation
LUA_FUNC(lua_FileExists, [](lua_State *L) {
	const char* filename = lua_tostring(L, 1);
	if (filename && filesystem->FileExists(filename)) {
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(lua_FileRead, [](lua_State *L) {
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

LUA_FUNC(lua_FileWrite, [](lua_State *L) {
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

LUA_FUNC(lua_CreateDir, [](lua_State *L) {
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

LUA_FUNC(lua_IsDir, [](lua_State *L) {
	const char* folder = lua_tostring(L, 1);
	if (folder && filesystem->IsDirectory(folder)) {
		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}
	return 1;
})

LUA_FUNC(lua_FindFiles, [](lua_State *L) {
	const char* wildcard = lua_tostring(L, 1);
	if (wildcard) {
		std::vector<std::string> files;
		GetFilesInDirectory("", wildcard, files);
		lua_newtable(L);
		for (size_t i = 0; i < files.size(); ++i) {
			lua_pushstring(L, files[i].c_str());
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	lua_pushnil(L);
	return 1;
})

LUA_FUNC(lua_DeleteFile, [](lua_State *L) {
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

LUA_FUNC(lua_RenameFile, [](lua_State *L) {
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

// sounds
LUA_FUNC(lua_PlaySound, [](lua_State *L) {
	const char* soundName = lua_tostring(L, 1);
	enginesound->EmitAmbientSound(soundName, 1.0f);
	return 0;
})

// other
LUA_FUNC(lua_IsLinux, [](lua_State *L) {
	lua_pushboolean(L, false); // dumbass its available in windows only
	return 1;
})

void MainLuaHandle::RegFunctions() {
	// server-related
	REG_FUNCTION(_StartNextLevel);
	REG_FUNCTION(_GetCurrentMap);
	REG_FUNCTION(_ServerCommand);
	REG_FUNCTION(_MaxPlayers);
	REG_FUNCTION(_Msg);
	REG_FUNCTION(_CurTime);
	REG_FUNCTION(_GetModPath);
	REG_FUNCTION(_IsDedicatedServer);
	REG_FUNCTION(_GetConVar_Float);
	REG_FUNCTION(_GetConVar_String);
	REG_FUNCTION(_GetConVar_Bool);

	// player-related
	REG_FUNCTION(_GiveAmmo);
	REG_FUNCTION(_GiveItem);

	// file manipulation
	REG_FUNCTION(_FileExists);
	REG_FUNCTION(_FileRead);
	REG_FUNCTION(_FileWrite);
	REG_FUNCTION(_CreateDir);
	REG_FUNCTION(_IsDir);
	REG_FUNCTION(_FindFiles);
	REG_FUNCTION(_DeleteFile);
	REG_FUNCTION(_RenameFile);

	// sounds
	REG_FUNCTION(_PlaySound);

	// other
	REG_FUNCTION(_IsLinux);
}

LuaHandle* g_LuaHandle = NULL;
LuaHandle* GetLuaHandle() {
	return g_LuaHandle;
}

MainLuaHandle::MainLuaHandle() : LuaHandle() {
	g_LuaHandle = this;
	Register();
}
