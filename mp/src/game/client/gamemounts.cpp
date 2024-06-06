#include "cbase.h"
#include "gamemounts.h"
#include "steam/steam_api.h"
#include "tier1/strtools.h"
#include "tier1/fmtstr.h"

#define MAX_LINE_LENGTH 256

void LoadAddons()
{
	const char *pszAddonDir = "addons/*";

	FileFindHandle_t findHandle;
	const char *pszAddonName = filesystem->FindFirst(pszAddonDir, &findHandle);

	if (!pszAddonName)
	{
		ConMsg("No addons found in directory: %s\n", pszAddonDir);
		return;
	}

	do
	{
		if (V_stricmp(pszAddonName, ".") != 0 && V_stricmp(pszAddonName, "..") != 0 && V_stricmp(pszAddonName, "checkers") != 0 && V_stricmp(pszAddonName, "chess") != 0
			&& V_stricmp(pszAddonName, "common") != 0 && V_stricmp(pszAddonName, "go") != 0 && V_stricmp(pszAddonName, "hearts") != 0 && V_stricmp(pszAddonName, "spades") != 0
			&& V_stricmp(pszAddonName, "??mportal") != 0 && V_stricmp(pszAddonName, "addons") != 0)
		{
			if (V_stricmp(pszAddonName, "maps") != 0 && V_stricmp(pszAddonName, "materials") != 0 && V_stricmp(pszAddonName, "models") != 0
				&& V_stricmp(pszAddonName, "particles") != 0 && V_stricmp(pszAddonName, "sound") != 0)
			{
				char szFullPath[MAX_PATH];
				Q_snprintf(szFullPath, sizeof(szFullPath), "addons/%s", pszAddonName);
				ConMsg("found addon dir: %s\n", szFullPath);
				MountAddon(pszAddonName);
				ConMsg("mounted addon: %s\n", pszAddonName);
			}
		}
		pszAddonName = filesystem->FindNext(findHandle);
	} while (pszAddonName);

	filesystem->FindClose(findHandle);
}


void MountAddon(const char *pszAddonName)
{
	char szPath[MAX_PATH];
	Q_snprintf(szPath, sizeof(szPath), "addons/%s", pszAddonName);

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/models", szPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/models", szPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/sound", szPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/sound", szPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/materials", szPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/materials", szPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/particles", szPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/particles", szPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/maps", szPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/maps", szPath), "GAME");

	g_pFullFileSystem->AddSearchPath(szPath, "GAME");}

void MountGames()
{
	if (!steamapicontext || !steamapicontext->SteamApps())
	{
		Warning("Steam API context is not available\n");
		return;
	}

	FileHandle_t hFile = filesystem->Open("cfg/mounts.cfg", "rt");

	if (!hFile)
	{
		Warning("failed to open mounts.cfg file\n");
		return;
	}

	char szLine[MAX_LINE_LENGTH];
	while (filesystem->ReadLine(szLine, sizeof(szLine), hFile))
	{
		if (szLine[0] == '\0' || szLine[0] == '/' || szLine[0] == '#')
			continue;

		int appID;
		char szGameName[MAX_LINE_LENGTH];
		char szPath[MAX_PATH * 2];
		if (sscanf(szLine, "%d : %s \"%[^\"]\"", &appID, szGameName, szPath) == 3)
		{
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_textures.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_misc.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_misc.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_pak_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/fallbacks_pak_dir.vpk", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s", szPath, szGameName), "GAME");

			ConMsg("successfuly mounted appID %d (%s)\n", appID, szGameName);

			//numGamesLoaded++;
		}
		else
		{
			Warning("invalid line format in mounts.cfg: %s\n", szLine);
			//numGamesLoaded = 0;
		}
	}

	//ConMsg("successfuly loaded %s games\n", numGamesLoaded);

	filesystem->Close(hFile);
}