#include "cbase.h"
#include "gamemounts.h"

#define MAX_LINE_LENGTH 256

// chill the fucking out im just prototyping
void MountGamesFromConfig()
{
	FileHandle_t hFile = filesystem->Open("cfg/mounts.cfg", "rt");

	if (!hFile)
	{
		Warning("Failed to open mounts.cfg file!\n");
		return;
	}

	char szLine[MAX_LINE_LENGTH];
	while (filesystem->ReadLine(szLine, sizeof(szLine), hFile) != NULL)
	{
		if (szLine[0] == '\0' || szLine[0] == '/' || szLine[0] == '#')
			continue;

		char szGame[MAX_LINE_LENGTH];
		char szPath[MAX_LINE_LENGTH];

		if (sscanf(szLine, "%s \"%[^\"]\"", szGame, szPath) == 2)
		{
			filesystem->AddSearchPath(szPath, szGame);
			ConMsg("mounted game %s from %s\n", szGame, szPath);

			char szMapsDir[MAX_PATH];
			Q_snprintf(szMapsDir, sizeof(szMapsDir), "%s/maps", szPath);
			filesystem->AddSearchPath(szMapsDir, "GAME");
			ConMsg("added maps directory for game %s\n", szGame);
		}
	}

	filesystem->Close(hFile);
}

void Mounts_Shutdown()
{

}