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

local gamemode = GAMEMODE

if gamemode == "Sandbox" then
    Msg("Hello Dad")
elseif gamemode == "Deathmatch" then
    Msg("Hello Mom")
else
    Msg("Hello World")
end