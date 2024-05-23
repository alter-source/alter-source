### Lua Functions

#### Server Functions
- `luaMsg(message)`
  - Print a message to the server console.
- `luaConMsg(message)`
  - Print a message to the server console.
- `luaGetTime()`
  - Get the current server time.
- `luaGetCVar(cvarName)`
  - Get the value of a ConVar.
- `luaSetCVar(cvarName, value)`
  - Set the value of a ConVar.
- `luaExecuteConsoleCommand(command)`
  - Execute a console command on the server.

#### Mod Functions
- `luaCreateCVar(name, defaultValue)`
  - Create a new ConVar.
- `luaCreateConCommand(name, description, callback)`
  - Create a new console command.
- `luaRegisterFunction(functionName, functionPtr)`
  - Register a Lua function.
- `luaSetGlobalVariable(variableName, value)`
  - Set a global Lua variable.

#### Player Functions
- `luaGetPlayerName(playerIndex)`
  - Get the name of a player.
- `luaGetPlayerHealth(playerIndex)`
  - Get the health of a player.
- `luaGetPlayerPosition(playerIndex)`
  - Get the position of a player.
- `luaHandlePlayerInput(playerIndex, input)`
  - Handle player input.
- `luaGetPlayerCount()`
  - Get the total number of players.
- `luaGetPlayerTeam(playerIndex)`
  - Get the team of a player.
- `luaGetPlayerMaxSpeed(playerIndex)`
  - Get the maximum speed of a player.
- `luaGiveItem(playerIndex, itemName)`
  - Give an item to a player.
- `luaIsPlayerValid(playerIndex)`
  - Check if a player is valid.
- `luaGetPlayerArmor(playerIndex)`
  - Get the armor of a player.
- `luaIsPlayerAlive(playerIndex)`
  - Check if a player is alive.
- `luaKillPlayer(playerIndex)`
  - Kill a player.
- `luaSetPlayerArmor(playerIndex, armorValue)`
  - Set the armor of a player.
- `luaGiveAmmo(playerIndex, ammoType, amount)`
  - Give ammo to a player.
- `luaSetPlayerHealth(playerIndex, health)`
  - Set the health of a player.
- `luaGetNewPlayer()`
  - Get the index of a new player.

#### Entity Functions
- `luaGetEntityPosition(entityIndex)`
  - Get the position of an entity.
- `luaGetEntityClassname(entityIndex)`
  - Get the classname of an entity.
- `luaSpawnEntity(classname, x, y, z)`
  - Spawn a new entity.
- `luaTeleportPlayer(playerIndex, x, y, z)`
  - Teleport a player to a position.
- `luaSetEntityProperty(entityIndex, property, value)`
  - Set a property of an entity.
- `luaIsPlayerNearEntity(playerIndex, entityIndex, radius)`
  - Check if a player is near an entity.
- `luaKillEntity(entityIndex)`
  - Kill an entity.
- `luaSetPlayerMaxSpeed(playerIndex, speed)`
  - Set the maximum speed of a player.
- `luaGetEntityName(entityIndex)`
  - Get the name of an entity.
- `luaGetEntityModel(entityIndex)`
  - Get the model of an entity.
- `luaGetEntityClassId(entityIndex)`
  - Get the class ID of an entity.
