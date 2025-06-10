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

# GameManager General Logic:
- shells would move before tanks (since they are faster),
- in a case a shell drops a wall at the same turn a tank move to this position, the wall,tank & shell would die.
- if a tank is trying to move into a wall on the turn of its destruction it will die as well.

# Map File Format
---------------------------
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

MapName: ignored title.
Rows, Cols, MaxSteps, NumShells: header values.
Grid characters:
. or space = empty
# = wall
@ = mine
1 = Player 1’s tank
2 = Player 2’s tank


# Code Structure
ArenaBattle/
    build/
        Makefile
    common/                  • shared interfaces (Player, TankAlgorithm, BattleInfo, SatelliteView, etc)
    include/
        Board.h              • grid and cell logic
        GameState.h          • core simulation steps
        GameManager.h        • I/O + game loop
        MyBattleInfo.h       • concrete BattleInfo for AIs
        MySatelliteView.h    • concrete SatelliteView for AIs
        MyPlayerFactory.h    • instantiates Player1/2
        MyTankAlgorithmFactory.h
        EvasiveTank.h
        AggressiveTank.h
        utils.h              • parseKeyValue helper
        Player1.h
        Player2.h
        Tank.h                 !!!! unused??? TODO: Remove that,

    src/
        *.cpp                • implementations
        main.cpp             • parses headers, builds factories, runs manager

