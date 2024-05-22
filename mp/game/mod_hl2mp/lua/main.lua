local gamemode = GAMEMODE

if gamemode == "Sandbox" then
    Say("Hello Dad")
elseif gamemode == "Deathmatch" then
    Say("Hello Mom")
else
    Say("Hello World")
end