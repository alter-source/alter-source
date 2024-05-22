-- Function: Msg(message: string)
-- Prints a message to the server console.
-- Parameters: message (string) - Message to print
-- Returns: None

-- Function: ConMsg(message: string)
-- Alias for 'Msg'. Prints a message to the server console.
-- Parameters: message (string) - Message to print
-- Returns: None

-- Function: ClientPrintAll(level: number, message: string, ...)
-- Prints a message to all clients.
-- Parameters: 
--    level (number) - Message level
--    message (string) - Message to print
--    ... (string) - Additional messages (optional)
-- Returns: None

-- Function: GetTime()
-- Retrieves the current server time.
-- Parameters: None
-- Returns: Current server time (number)

-- Function: GetCVar(cvarName: string)
-- Retrieves the value of a ConVar.
-- Parameters: cvarName (string) - Name of the ConVar
-- Returns: Value of the ConVar (string) or nil if ConVar not found

-- Function: SetCVar(cvarName: string, cvarValue: string)
-- Sets the value of a ConVar.
-- Parameters: 
--    cvarName (string) - Name of the ConVar
--    cvarValue (string) - New value for the ConVar
-- Returns: None

-- Function: ExecuteConsoleCommand(command: string)
-- Executes a console command on the server.
-- Parameters: command (string) - Command to execute
-- Returns: None


-- VARIABLES


-- Variable: FOR_ALL_PLAYERS
-- Indicates that an action should be applied to all players.
-- Type: number

-- Variable: INVALID_ENTITY
-- Represents an invalid entity.
-- Type: number

-- Variable: NULL
-- Represents a null value.
-- Type: number

-- Variable: GAMEMODE
-- Current game mode name.
-- Type: string

-- Variable: TEAMPLAY
-- Indicates whether team play is enabled.
-- Type: number

-- Variable: COOP
-- Indicates whether cooperative play is enabled.
-- Type: number

-- Variable: WEAPON_MELEE_SLOT
-- Slot number for melee weapons.
-- Type: number

-- Variable: WEAPON_SECONDARY_SLOT
-- Slot number for secondary weapons.
-- Type: number

-- Variable: WEAPON_PRIMARY_SLOT
-- Slot number for primary weapons.
-- Type: number

-- Variable: WEAPON_EXPLOSIVE_SLOT
-- Slot number for explosive weapons.
-- Type: number

-- Variable: WEAPON_TOOL_SLOT
-- Slot number for tools.
-- Type: number

-- Variable: MAX_FOV
-- Maximum field of view allowed.
-- Type: number

-- Variable: MAX_HEALTH
-- Maximum health a player can have.
-- Type: number

-- Variable: MAX_ARMOR
-- Maximum armor a player can have.
-- Type: number

-- Variable: MAX_PLAYERS
-- Maximum number of players allowed in the game.
-- Type: number

-- Variable: TEAM_NONE
-- Team ID for players not assigned to any team.
-- Type: number

-- Variable: TEAM_SPECTATOR
-- Team ID for spectators.
-- Type: number

-- Variable: TEAM_ANY
-- Team ID for any team.
-- Type: number

-- Variable: TEAM_INVALID
-- Team ID for an invalid or unspecified team.
-- Type: number

-- Variable: TEAM_ONE
-- Team ID for the first team (e.g., Combine in Half-Life 2).
-- Type: number

-- Variable: TEAM_TWO
-- Team ID for the second team (e.g., Rebels in Half-Life 2).
-- Type: number

-- Variable: HUD_PRINTNOTIFY
-- Enumeration value for printing a notify message in the HUD.
-- Type: number

-- Variable: HUD_PRINTCONSOLE
-- Enumeration value for printing a message in the console.
-- Type: number

-- Variable: HUD_PRINTTALK
-- Enumeration value for printing a talk message in the HUD.
-- Type: number

-- Variable: HUD_PRINTCENTER
-- Enumeration value for printing a center message in the HUD.
-- Type: number

-- ps. the plot armor keeps me safe