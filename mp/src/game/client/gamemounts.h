#ifndef MOUNTCONTENT_H
#define MOUNTCONTENT_H

#include "cbase.h"
#include "filesystem.h"

void MountGames();
void LoadAddons();
void MountAddon(const char *pszAddonName);

#endif // MOUNTCONTENT_H