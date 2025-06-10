# ArenaBattle Tank Game
A turn-based, toroidal grid “tank battle” simulator in C++20, where two players control fleets of tanks that move, rotate, shoot shells, trigger mines, and smash walls. The game supports plug-n-play GameManager, tank AI via two built-in algorithms:

 - AggressiveTank: Seeks line-of-sight shots, breaks walls (if enough shells), otherwise advances.

 - EvasiveTank: Fetches fresh battlefield info each turn, predicts shell trajectories, and flees to maximize distance.

# Features
* Toroidal wrap for shells (but tanks may optionally wrap).
* Two-cell shell movement per tick with mid-step collision checks.
* Walls require two hits to break.
* Mines detonate on tank contact.
* Head-on & multi-tank collisions kill all involved.
* Pluggable AI via common::TankAlgorithm & common::Player interfaces.
* Per-action logging to an output file.

# Requirements
- C++20 compiler (e.g. clang or gcc).
- CMake ≥ 3.10.

# Building
cd build
make

# Usage
./tanks_game <map_file.txt>

# Map File Format
Plain text, e.g. basic.txt:
---------------------------
<MapName>
Rows = R
Cols = C
MaxSteps = M
NumShells = S

<grid row 0>
<grid row 1>
…
<grid row R-1>
---------------------------
MapName: ignored title.
Rows, Cols, MaxSteps, NumShells: header values.
Grid characters:
. or space = empty
# = wall
@ = mine
1 = Player 1’s tank
2 = Player 2’s tank


# Game Manager General Logic:
- shells would move before tanks (since they are faster),
- in a case a shell drops a wall at the same turn a tank move to this position, the wall,tank & shell would die.
- if a tank is trying to move into a wall on the turn of its destruction it will die as well.

# Game Log 
Logs Tanks operations (in the order the tanks reads from the board)
e.g. 4 tank (2 tanks for each player):
---------------------------------------------------------------------------------------
<row-1>: <TankAI(1,0),TankAI(1,1),...,TankAI(2,0),TankAI(2,1)
<row-2>: <TankAI(1,0),TankAI(1,1),...,TankAI(2,0),TankAI(2,1)
  ...
<row-last-round>: <TankAI(1,0),TankAI(1,1),...,TankAI(2,0),TankAI(2,1)
<game_result_message>
<row-2>:
---------------------------------------------------------------------------------------
Raw Text:
---------------------------------------------------------------------------------------
GetBattleInfo, GetBattleInfo, GetBattleInfo, GetBattleInfo
MoveBackward, RotateRight45, MoveBackward, MoveForward
GetBattleInfo, RotateRight45, GetBattleInfo, MoveForward
MoveBackward, RotateRight45, MoveForward, MoveForward
GetBattleInfo, RotateRight45, GetBattleInfo, MoveForward
MoveBackward (killed), MoveForward (killed), MoveBackward, MoveForward
killed, killed, GetBattleInfo, MoveForward
killed, killed, MoveForward, MoveForward
killed, killed, GetBattleInfo (killed), MoveForward (killed)
Tie, both players have zero tanks
---------------------------------------------------------------------------------------

# Input Error Case
Printing an error message

# Code Structure
Code Tree Directory Structure:
------------------------------
ArenaBattle/
├── build/
│   ├── *.o [after-make-build]
├── common/
│   ├── ActionRequest.h
│   ├── TankAlgorithm.h
│   ├── BattleInfo.h
│   ├── SatelliteView.h
│   ├── Player.h
│   ├── PlayerFactory.h
│   └── TankAlgorithmFactory.h
├── include/
│   ├── Board.h
│   ├── GameState.h
│   ├── GameManager.h
│   ├── MyTankAlgorithm.h
│   ├── MyBattleInfo.h
│   ├── AggressiveTank.h
│   ├── EvasiveTank.h
│   ├── MyPlayerFactory.h
│   ├── MyTankAlgorithmFactory.h
│   ├── Player1.h
│   └── Player2.h
│   └── MySatelliteView.h
|   |__ utils.h
└── src/
    ├── AggressiveTank.cpp
    ├── EvasiveTank.cpp
    ├── GameManager.cpp
    ├── Player1.cpp
    ├── Player2.cpp
    ├── MyTankAlgorithm.cpp
    ├── MyBattleInfo.cpp
    ├── MySatelliteView.cpp
    ├── Board.cpp
    ├── GameState.cpp
    ├── utils.cpp
    ├── MyTankAlgorithmFactory.cpp
    ├── MyPlayerFactory.cpp
    └── main.cpp
