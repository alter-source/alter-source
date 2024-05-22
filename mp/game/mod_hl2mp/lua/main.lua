local gamemode = GAMEMODE

if gamemode == "Sandbox" then
    Msg("Hello Dad")
elseif gamemode == "Deathmatch" then
    Msg("Hello Mom")
else
    Msg("Hello World")
end