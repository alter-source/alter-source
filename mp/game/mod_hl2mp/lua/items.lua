local player = GetNewPlayer()

local function GiveItems(ammoList, itemList)
    for _, ammo in ipairs(ammoList) do
        GiveAmmo(player, ammo.amount, ammo.type)
    end
    for _, item in ipairs(itemList) do
        GiveItem(player, item)
    end
end

local function SetGameModeSettings(mode)
    if mode == "Sandbox" then
        SetCVar("sv_infinite_aux_power", 1)
        GiveItems(
            {
                {amount = 255, type = "Pistol"},
                {amount = 255, type = "AR2"},
                {amount = 5, type = "AR2AltFire"},
                {amount = 255, type = "SMG1"},
                {amount = 1, type = "smg1_grenade"},
                {amount = 255, type = "Buckshot"},
                {amount = 32, type = "357"},
                {amount = 3, type = "rpg_round"},
                {amount = 1, type = "grenade"},
                {amount = 2, type = "slam"}
            },
            {
                "weapon_crowbar", "weapon_stunstick", "weapon_pistol", "weapon_357",
                "weapon_smg1", "weapon_ar2", "weapon_shotgun", "weapon_frag",
                "weapon_crossbow", "weapon_rpg", "weapon_slam", "weapon_physcannon",
                "weapon_physgun", "weapon_bugbait"
            }
        )
    elseif mode == "Cooperative" then
        SetCVar("sv_infinite_aux_power", 0)
        GiveItems(
            {
                {amount = 255, type = "Pistol"},
                {amount = 45, type = "SMG1"},
                {amount = 1, type = "grenade"},
                {amount = 6, type = "Buckshot"},
                {amount = 6, type = "357"}
            },
            {
                "weapon_stunstick", "weapon_crowbar", "weapon_pistol", "weapon_smg1",
                "weapon_frag", "weapon_physcannon"
            }
        )
    elseif mode == "Deathmatch" or mode == "TDM" then
        SetCVar("sv_infinite_aux_power", 0)
        GiveItems(
            {
                {amount = 255, type = "Pistol"},
                {amount = 45, type = "SMG1"},
                {amount = 1, type = "grenade"},
                {amount = 6, type = "Buckshot"},
                {amount = 6, type = "357"}
            },
            {
                "weapon_crowbar", "weapon_pistol", "weapon_smg1", "weapon_frag",
                "weapon_physcannon"
            }
        )
    elseif mode == "Chaotic" then
        SetCVar("sv_infinite_aux_power", 0)
        GiveItems({}, {"weapon_crowbar"})
    end
end

SetGameModeSettings(GAMEMODE)
