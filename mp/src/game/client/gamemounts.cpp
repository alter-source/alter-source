#include "cbase.h"
#include "gamemounts.h"
#include "steam/steam_api.h"
#include "tier1/strtools.h"
#include "tier1/fmtstr.h"
#include "lua/luahandle.h"
#include "filesystem.h"

#include "vgui/IVGui.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Frame.h"
#include <vgui/ISurface.h>
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/ScrollBar.h"

#include <vector>
#include <string>

#include <map>
#include <string>
#include <functional>

#include "tier0/memdbgon.h"

#define MAX_LINE_LENGTH 256

ConVar portal_mounted("portal_mounted", "0", FCVAR_REPLICATED, "Indicates if Portal is mounted");
ConVar tf2_mounted("tf2_mounted", "0", FCVAR_REPLICATED, "Indicates if Team Fortress 2 is mounted");

extern ConVar hl2_episodic; // indicates if episode one is mounted
ConVar hl2_ep2("ep2_mounted", "0", FCVAR_REPLICATED, "Indicates if Half-Life 2: Episode Two is mounted");
ConVar hl2_lc("lc_mounted", "0", FCVAR_REPLICATED, "Indicates if Half-Life 2: Lost Coast is mounted");
ConVar hl1_src("hl_source_mounted", "0", FCVAR_REPLICATED, "Indicated if Half Life: Source is mounted");

struct AddonInfo {
	std::string name;
	std::string description;
};

std::vector<AddonInfo> g_Addons;

char* GetPath() {
	auto* gameDirectory = engine->GetGameDirectory();
	char* addonPath = "\\addons";
	size_t totalLength = strlen(gameDirectory) + strlen(addonPath) + 1;
	char* result = (char*)malloc(totalLength);

	if (result != NULL) {
		strcpy(result, gameDirectory);
		strcat(result, addonPath);
		return result;
	}
	else {
		Error("memory allocation error");
	}
}

void ReadAddonInfo(const char* addonDir) {
	char szAddonInfoPath[MAX_PATH];
	Q_snprintf(szAddonInfoPath, sizeof(szAddonInfoPath), "%s\\addon-info.txt", addonDir);

	if (filesystem->FileExists(szAddonInfoPath)) {
		FileHandle_t hFile = filesystem->Open(szAddonInfoPath, "r", "GAME");
		if (hFile) {
			AddonInfo addonInfo;

			char szLine[MAX_LINE_LENGTH];
			filesystem->ReadLine(szLine, sizeof(szLine), hFile);
			if (szLine[0] != '\0') {
				addonInfo.name = szLine;
			}
			else {
				addonInfo.name = "Unnamed Addon";
			}

			filesystem->ReadLine(szLine, sizeof(szLine), hFile);
			if (szLine[0] != '\0') {
				addonInfo.description = szLine;
			}
			else {
				addonInfo.description = "No description available";
			}

			g_Addons.push_back(addonInfo);

			filesystem->Close(hFile);
		}
		else {
			Warning("Failed to open addon-info.txt for reading: %s\n", szAddonInfoPath);
		}
	}
	else {
		Warning("Addon info file not found: %s\n", szAddonInfoPath);
	}
}

char* GetAddonName(const char* szFullPath) {
	char* fullPath = strdup(szFullPath);
	char* addonName = nullptr;
	char* lastSeparator = strrchr(fullPath, '\\');
	if (!lastSeparator)
		lastSeparator = strrchr(fullPath, '/');
	if (lastSeparator)
		addonName = strdup(lastSeparator + 1);
	else
		addonName = strdup(fullPath);
	free(fullPath);
	return addonName;
}

void LoadAddons() {
	char* addonsFolder = GetPath();
	if (!addonsFolder) {
		Warning("addon folder not found\n");
		return;
	}

	const char *pszAddonDir = "addons\\*";
	FileFindHandle_t findHandle;
	const char *pszAddonName = filesystem->FindFirst(pszAddonDir, &findHandle);

	if (!pszAddonName) {
		ConMsg("no addons found in directory: %s\n", pszAddonDir);
		return;
	}

	do {
		if (V_stricmp(pszAddonName, ".") != 0 && V_stricmp(pszAddonName, "..") != 0
			&& V_stricmp(pszAddonName, "checkers") != 0 && V_stricmp(pszAddonName, "chess") != 0
			&& V_stricmp(pszAddonName, "common") != 0 && V_stricmp(pszAddonName, "go") != 0
			&& V_stricmp(pszAddonName, "hearts") != 0 && V_stricmp(pszAddonName, "spades") != 0
			&& V_stricmp(pszAddonName, "ð„Ómportal") != 0 && V_stricmp(pszAddonName, "addons") != 0
			&& V_stricmp(pszAddonName, "maps") != 0 && V_stricmp(pszAddonName, "materials") != 0
			&& V_stricmp(pszAddonName, "sound") != 0 && V_stricmp(pszAddonName, "models") != 0
			&& V_stricmp(pszAddonName, "particles") != 0) {


			char szFullPath[MAX_PATH];
			Q_snprintf(szFullPath, sizeof(szFullPath), "%s\\%s", addonsFolder, pszAddonName);

			if (filesystem->IsDirectory(szFullPath)) {
				ConMsg("found addon dir: %s\n", szFullPath);
				MountAddon(szFullPath);
				ReadAddonInfo(szFullPath);
			}
			else {
				if (V_stristr(pszAddonName, ".as")) {
					ConMsg("found addon file: %s\n", szFullPath);

					char tempFolder[MAX_PATH];
					Q_snprintf(tempFolder, sizeof(tempFolder), "%s//%s", getenv("TEMP"), pszAddonName);

					char dirPath[MAX_PATH];
					Q_strncpy(dirPath, szFullPath, strrchr(szFullPath, '\\') - szFullPath + 1);
					dirPath[strrchr(szFullPath, '\\') - szFullPath + 1] = '\0';

					char extractCmd[MAX_PATH * 2];
					Q_snprintf(extractCmd, sizeof(extractCmd), "%s7z.exe x \"%s\" -o\"%s\\bin\"", dirPath, szFullPath, tempFolder);

					FILE* pipe = _popen(extractCmd, "r");
					if (!pipe) {
						ConMsg("Error executing command.\n");
						return;
					}

					char buffer[128];
					while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
						Msg("%s", buffer);
					}

					_pclose(pipe);

					char* lastDot = strrchr(tempFolder, '.');
					if (lastDot != NULL && strcmp(lastDot, ".as") == 0) {
						*lastDot = '\0';
					}

					ConMsg("extracted addon to: %s\n", tempFolder);

					MountAddon(tempFolder);
					ReadAddonInfo(szFullPath);
				}
			}
		}
		pszAddonName = filesystem->FindNext(findHandle);
	} while (pszAddonName);

	filesystem->FindClose(findHandle);
}


void MountAddon(const char *pszAddonName) {
	char szFullPath[MAX_PATH];
	Q_snprintf(szFullPath, sizeof(szFullPath), "%s", pszAddonName);

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/models", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/models", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/sound", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/sound", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/materials", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/materials", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/particles", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/particles", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/maps", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/maps", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(szFullPath, "GAME");
}

void MountGames() {
	if (!steamapicontext || !steamapicontext->SteamApps()) {
		Warning("Steam API context is not available\n");
		return;
	}

	FileHandle_t hFile = filesystem->Open("cfg/mounts.cfg", "rt");

	if (!hFile) {
		Warning("failed to open cfg/mounts.cfg file\n");
		return;
	}

	int numGamesLoaded = 0;

	char szLine[MAX_LINE_LENGTH];
	while (filesystem->ReadLine(szLine, sizeof(szLine), hFile)) {
		if (szLine[0] == '\0' || szLine[0] == '/' || szLine[0] == '#' || szLine[0] == ' ' || szLine[0] == '{' || szLine[0] == '}' || Q_strnicmp(szLine, "\"mounts\"", 8) == 0)
			continue;

		int appID;
		char szGameName[MAX_LINE_LENGTH];
		char newGameName[MAX_LINE_LENGTH];
		char szPath[MAX_PATH * 2];
		if (sscanf(szLine, "%d : %s \"%[^\"]\"", &appID, szGameName, szPath) == 3) {
			std::map<std::string, std::function<void(int)>> gameMapping = {
				{ "portal", [](int val) { portal_mounted.SetValue(val); } },
				{ "tf", [](int val) { tf2_mounted.SetValue(val); } },
				{ "episodic", [](int val) { hl2_episodic.SetValue(val); } },
				{ "ep2", [](int val) { hl2_ep2.SetValue(val); } },
				{ "lostcoast", [](int val) { hl2_lc.SetValue(val); } },
				{ "hl1", [](int val) { hl1_src.SetValue(val); } }
			};

			auto it = gameMapping.find(szGameName);
			if (it != gameMapping.end()) {
				it->second(1);
			}

			if (strcmp(szGameName, "tf") == 0) {
				strcpy(newGameName, "tf2");
			}
			else {
				strcpy(newGameName, szGameName);
			}

			Msg("%s at %s\n", szGameName, newGameName);

			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_textures.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_misc.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_misc.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_pak_dir.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_textures_dir.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_misc_dir.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_dir.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_english_dir.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_russian_dir.vpk", szPath, szGameName, newGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s", szPath, szGameName), "GAME");

			ConMsg("successfuly mounted appID %d (%s)\n", appID, szGameName);
			numGamesLoaded++;
		}
	}

	ConMsg("successfuly loaded %d game(s)\n", numGamesLoaded);
	filesystem->Close(hFile);
}

class CAddonListPanel : public vgui::Frame {
	DECLARE_CLASS_SIMPLE(CAddonListPanel, vgui::Frame);
public:
	CAddonListPanel(vgui::VPANEL parent);
	virtual void Activate();

private:
	void PopulateAddonList();
	vgui::ListPanel *m_pAddonList;
	vgui::ScrollBar *m_pScrollBar;
};

CAddonListPanel::CAddonListPanel(vgui::VPANEL parent) : BaseClass(nullptr, "AddonListPanel") {
	SetParent(parent);
	SetTitle("Installed Addons", true);
	SetSize(400, 600);
	SetSizeable(false);
	SetDeleteSelfOnClose(true);
	SetVisible(false);

	m_pAddonList = new vgui::ListPanel(this, "AddonList");
	m_pAddonList->SetBounds(10, 30, 380, 560);
	m_pAddonList->AddColumnHeader(0, "addonName", "Addon Name", 200);
	m_pAddonList->AddColumnHeader(1, "description", "Description", 300);
	m_pAddonList->SetMultiselectEnabled(false);

	//LoadControlSettings("resource/ui/AddonListPanel.res");
}

void CAddonListPanel::Activate() {
	BaseClass::Activate();
	PopulateAddonList();
}

void CAddonListPanel::PopulateAddonList() {
	m_pAddonList->DeleteAllItems();

	for (const auto& addon : g_Addons) {
		KeyValues *kv = new KeyValues("AddonItem");
		kv->SetString("addonName", addon.name.c_str());
		kv->SetString("description", addon.description.c_str());
		m_pAddonList->AddItem(kv, 0, false, false);
		kv->deleteThis();
	}

	m_pAddonList->SortList();
}

CON_COMMAND(show_addons, "Show installed addons") {
	CAddonListPanel *pPanel = new CAddonListPanel(vgui::surface()->GetEmbeddedPanel());
	pPanel->Activate();
}