# Lua Function Documentation

## General Information
These Lua functions provide an interface for interacting with game entities, players, console commands, and more in a Lua scripting environment.

## Functions

### Printing Messages
- `Msg(msg: string)`
  - Prints a message to the server console.
- `ConMsg(msg: string)`
  - Prints a message to the player's console.

### Time and Variables
- `GetTime() -> number`
  - Returns the current game time.
- `GetCVar(name: string) -> string`
  - Retrieves the value of a console variable.
- `SetCVar(name: string, value: string)`
  - Sets the value of a console variable.
  
### Console Commands and Execution
- `ExecuteConsoleCommand(command: string)`
  - Executes a console command on the server.
- `Say(message: string)`
  - Sends a message to all players.

### File Operations
- `LoadFile(filePath: string)`
  - Loads and executes a Lua script file.
  
### Entity and Player Manipulation
- `GetPlayerName(index: number) -> string`
  - Retrieves the name of the player by index.
- `GetPlayerHealth(index: number) -> number`
  - Retrieves the health of the player by index.
- `GetPlayerPosition(index: number) -> number, number, number`
  - Retrieves the position of the player by index.
- `TeleportPlayer(index: number, x: number, y: number, z: number)`
  - Teleports the player to the specified coordinates.
- `KillPlayer(index: number)`
  - Kills the player.
- `GetEntityPosition(index: number) -> number, number, number`
  - Retrieves the position of the entity by index.

### Mathematical Operations
- `AddNumbers(num1: number, num2: number) -> number`
  - Adds two numbers.
- `SubtractNumbers(num1: number, num2: number) -> number`
  - Subtracts one number from another.
- `MultiplyNumbers(num1: number, num2: number) -> number`
  - Multiplies two numbers.
- `DivideNumbers(num1: number, num2: number) -> number`
  - Divides one number by another.
- `IsNumberPositive(num: number) -> boolean`
  - Checks if a number is positive.
- `IsNumberNegative(num: number) -> boolean`
  - Checks if a number is negative.
- `IsNumberZero(num: number) -> boolean`
  - Checks if a number is zero.

### String Operations
- `StringLength(str: string) -> number`
  - Returns the length of a string.
- `ConcatStrings(...) -> string`
  - Concatenates multiple strings.
- `StringCompare(str1: string, str2: string) -> boolean`
  - Compares two strings.
- `StringSubstring(str: string, start: number, length: number) -> string`
  - Retrieves a substring from a string.
- `StringFind(str: string, pattern: string) -> number`
  - Finds a pattern in a string.
- `StringReplace(str: string, target: string, replacement: string) -> string`
  - Replaces occurrences of a substring in a string.
- `IsStringEmpty(str: string) -> boolean`
  - Checks if a string is empty.

### Table Operations
- `TableHasKey(table: table, key: any) -> boolean`
  - Checks if a table contains a specific key.
- `TableMerge(table1: table, table2: table)`
  - Merges two tables.
- `TableClear(table: table)`
  - Clears all elements from a table.
- `TableCopy(table: table) -> table`
  - Creates a shallow copy of a table.
- `TableRemoveAt(table: table, index: number)`
  - Removes an element at a specific index from a table.
- `TableForEach(table: table, func: function)`
  - Applies a function to each element of a table.
- `TableSort(table: table, func: function)`
  - Sorts a table using a comparison function.
- `TableSlice(table: table, start: number, length: number) -> table`
  - Returns a portion of a table.
- `TablePush(table: table, ...)`
  - Pushes one or more values onto the end of a table.
- `TablePop(table: table)`
  - Removes and returns the last element of a table.
- `TableConcat(table: table) -> string`
  - Concatenates all string values in a table.

### Other Utility Functions
- `GetNewPlayer() -> number`
  - Retrieves the index of the last player.

### Entity Creation and Manipulation
- `SpawnEntity(classname: string, x: number, y: number, z: number)`
  - Spawns an entity of the specified class at the given coordinates.
- `SetEntityProperty(index: number, property: string, ...)`
  - Sets various properties of an entity.
- `KillEntity(index: number)`
  - Removes an entity from the game.

### Player-Specific Functions
- `HandlePlayerInput(index: number, action: string)`
  - Handles player input, such as jumping.
- `GiveItem(index: number, itemName: string)`
  - Gives an item to the player.
- `SetPlayerArmor(index: number, armor: number)`
  - Sets the armor value for the player.
- `IsPlayerValid(index: number) -> boolean`
  - Checks if a player with the given index exists.
- `IsPlayerAlive(index: number) -> boolean`
  - Checks if the player is alive.
- `SetPlayerHealth(index: number, health: number)`
  - Sets the health of the player.

### Utility Functions
- `SetLastPlayerIndex(index: number)`
  - Sets the last player index.

### Advanced Entity and Player Queries
- `IsPlayerNearEntity(playerIndex: number, entityIndex: number, radius: number) -> boolean`
  - Checks if a player is within a certain radius of an entity.
- `GetEntityByClassname(classname: string) -> number`
  - Retrieves the index of an entity by its classname.
- `TraceLine(startX: number, startY: number, startZ: number, endX: number, endY: number, endZ: number) -> number, number, number`
  - Traces a line between two points and returns the hit position.

### Additional Entity Manipulation
- `SetEntityModel(index: number, modelName: string)`
  - Sets the model of an entity.
- `SetEntityAngles(index: number, pitch: number, yaw: number, roll: number)`
  - Sets the angles of an entity.

### Ammo and Item Functions
- `GiveAmmo(playerIndex: number, ammoType: string, amount: number)`
  - Gives ammo to a player.

## Note
- These functions provide a comprehensive set of tools for scripting and modding within the game environment.
- Ensure proper error handling and validation when using these functions to avoid unexpected behavior or crashes.
