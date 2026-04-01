# Maze Game Flow Diagram (ASCII)

## Overview

TitleLevel Matchmaking -> MazeLevel Wait -> Countdown + Maze Gen -> Teleport -> Goal Reached -> Return to TitleLevel

---

## 1. Full Game Flow (Sequence Diagram)

```
  Host (Listen Server)           Client                    AMazeGameMode             AMazeGameState            UMazeGenerator           AMazeGoalActor
        |                           |                           |                          |                         |                        |
        |  ===================== Phase 1: TitleLevel / Matchmaking ======================  |                         |                        |
        |                           |                           |                          |                         |                        |
        |--[UTitleWidget]           |                           |                          |                         |                        |
        |  GameStart Click          |                           |                          |                         |                        |
        |--[UMatchWidget Show]      |                           |                          |                         |                        |
        |--CreateLobby              |                           |                          |                         |                        |
        |  SOSManager               |                           |                          |                         |                        |
        |  .CreateSession(Max=4)    |                           |                          |                         |                        |
        |--ServerTravel             |                           |                          |                         |                        |
        |  (TitleLevel?listen)      |                           |                          |                         |                        |
        |--UIFlow.SetScreenLobby    |                           |                          |                         |                        |
        |  (host=true)              |                           |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |                           |--FindLobby                |                          |                         |                        |
        |                           |  SOSManager.FindSessions  |                          |                         |                        |
        |                           |--JoinLobby                |                          |                         |                        |
        |                           |  SOSManager.JoinSession   |                          |                         |                        |
        |<---ClientTravel-----------|  ByIndex()                |                          |                         |                        |
        |                           |--UIFlow.SetScreenLobby    |                          |                         |                        |
        |                           |  (host=false)             |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |  ========================= Phase 2: Lobby Wait ============================      |                         |                        |
        |                           |                           |                          |                         |                        |
        |--[ULobbyWidget]           |--[ULobbyWidget]           |                          |                         |                        |
        |  StartBtn + MazeSize      |  ReadyBtn only            |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |<--ServerSetReady(true)----|                           |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |--All Players Ready Check  |                           |                          |                         |                        |
        |--LobbyGS.SetGameStarted   |                           |                          |                         |                        |
        |  (true) --> Replicate---->|                           |                          |                         |                        |
        |--MazeSession              |                           |                          |                         |                        |
        |  .SetExpectedPlayers(N)   |                           |                          |                         |                        |
        |--Session:                 |                           |                          |                         |                        |
        |  AllowJoinInProgress=false|                           |                          |                         |                        |
        |--ServerTravel             |                           |                          |                         |                        |
        |  (MazeLevel?listen        |                           |                          |                         |                        |
        |   ?MazeSize=X)            |                           |                          |                         |                        |
        |~~Auto Travel~~~~~~~~~~~~> |                           |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |  ==================== Phase 3: MazeLevel Player Wait ========================    |                         |                        |
        |                           |                           |                          |                         |                        |
        |--PostLogin(Player)--------------------------->|       |                          |                         |                        |
        |                           |--PostLogin(Player)------->| ArrivedPlayers.Add       |                         |                        |
        |                           |                           |                          |                         |                        |
        |                           |                           |--TryStartGameFlow()      |                         |                        |
        |                           |                           |  [ArrivedPlayers         |                         |                        |
        |                           |                           |   >= ExpectedCount?]     |                         |                        |
        |                           |                           |   YES --> Start Flow     |                         |                        |
        |                           |                           |   NO  --> 30s Timeout    |                         |                        |
        |                           |                           |          then ForceStart |                         |                        |
        |                           |                           |                          |                         |                        |
        |  ================ Phase 4: Maze Gen + Countdown (10s) =======================    |                         |                        |
        |                           |                           |                          |                         |                        |
        |                           |                           |--GenerateAndSpawnMaze()--------------------------> |                        |
        |                           |                           |                          |                         |--Random Seed Gen       |
        |                           |                           |                          |                         |--BuildMazeGrid         |
        |                           |                           |                          |                         |  (Kruskal Algorithm)   |
        |                           |                           |                          |<--SetMazeData-----------|                        |
        |                           |                           |                          |  (Seed,Width,Height)    |                        |
        |                           |<--OnRep_MazeSeed----------|                          |                         |                        |
        |                           |  Client Wall Spawn        |                          |--SpawnWallsWithDelay    |                        |
        |                           |                           |                          |  (0.02s interval)       |                        |
        |                           |                           |                          |--OnWallSpawnComplete    |                        |
        |                           |                           |                          |--SpawnGameplayActors    |                        |
        |                           |                           |                          |  Goal(center)           |                        |
        |                           |                           |                          |  TargetPoint(corners)   |                        |
        |                           |                           |                          |  Bot(x4)                |                        |
        |                           |                           |                          |                         |                        |
        |                           |                           |--Phase=Countdown-------->|                         |                        |
        |                           |                           |  CountdownEndTime Set    |                         |                        |
        |                           |<--OnRep_Phase-------------|                          |                         |                        |
        |                           |  [MazeCountdownWidget     |                          |                         |                        |
        |                           |   Show 10s]               |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |  ====================== Phase 5: Teleport + Play ============================    |                         |                        |
        |                           |                           |                          |                         |                        |
        |                           |                           |--TeleportPlayers()       |                         |                        |
        |                           |                           |  Each Player -->         |                         |                        |
        |                           |                           |  MazeTargetPoint         |                         |                        |
        |                           |                           |--Phase=Playing---------->|                         |                        |
        |                           |<--OnRep_Phase-------------|                          |                         |                        |
        |                           |  Countdown Remove         |                          |                         |                        |
        |                           |  BGM Start                |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |  ==================== Phase 6: Goal Reached + Match End ======================   |                         |                        |
        |                           |                           |                          |                         |                        |
        |                           |--[Pawn Overlap]----------------------------------------------------------------------->|                |
        |                           |                           |<--OnGoalReached(WinnerPC)-------------------------------|                   |
        |                           |                           |--WinnerPlayer Set------->|                         |                        |
        |                           |                           |--Phase=GameOver--------->|                         |                        |
        |                           |<--OnRep_MatchResult-------|                          |                         |                        |
        |                           |  BGM Stop                 |                          |                         |                        |
        |                           |  Win/Lose Sound           |                          |                         |                        |
        |                           |  [UCommonModalWidget]     |                          |                         |                        |
        |                           |                           |                          |                         |                        |
        |  ================= Phase 7: Return to TitleLevel (3s) ========================   |                         |                        |
        |                           |                           |                          |                         |                        |
        |                           |                           |--ReturnToLobbyDelay      |                         |                        |
        |                           |                           |  (3s Timer)              |                         |                        |
        |                           |                           |--Session:                |                         |                        |
        |                           |                           |  AllowJoinInProgress     |                         |                        |
        |                           |                           |  = true                  |                         |                        |
        |                           |                           |--MazeSession             |                         |                        |
        |                           |                           |  .SetMatchStarted(false) |                         |                        |
        |                           |                           |--ServerTravel            |                         |                        |
        |                           |                           |  (TitleLevel?listen)     |                         |                        |
        |<~~~~~~~~~~~~~~~~~~~~~~~~~~|<~~~~~~~~~~~~~~~~~~~~~~~~~~|                          |                         |                        |
        |  Auto Travel --> TitleLevel (LobbyWidget State)       |                          |                         |                        |
        |                           |                           |                          |                         |                        |
```

---

## 2. Game Phase State Machine

```
    +================================================================+
    |                        TitleLevel                              |
    |                                                                |
    |  +----------------+    GameStart    +------------------+       |
    |  |  UTitleWidget  |---------------> |   UMatchWidget   |       |
    |  | (Title Screen) |<--------------- |  (Matchmaking)   |       |
    |  +----------------+   Exit Click    +------------------+       |
    |                                      |                         |
    |                               Create/Join Lobby                |
    |                                      |                         |
    |                                      v                         |
    |                            +------------------+                |
    |                            |   ULobbyWidget   |<----------+    |
    |                            |   (Lobby Wait)   |           |    |
    |                            +------------------+           |    |
    +========================|==============================|===|====+
                             |                              |   |
                  Host: ServerTravel              3s ReturnToLobby()
                (MazeLevel?listen?MazeSize=X)    ServerTravel(TitleLevel)
                             |                              |
                             v                              |
    +================================================================+
    |                        MazeLevel                               |
    |                                                                |
    |  +---------------------+  All Players Arrived  +-----------+   |
    |  | WaitingForPlayers   |--------------------->| Countdown  |   |
    |  | (Wait for Connect,  |  OR 30s Timeout      | (10s Timer |   |
    |  |  30s Timeout)       |                      |  + MazeGen)|   |
    |  +---------------------+                      +-----------+    |
    |                                                    |           |
    |                                         Countdown End          |
    |                                        TeleportPlayers()       |
    |                                                    |           |
    |                                                    v           |
    |                                               +-----------+    |
    |                        Player Reaches Goal    |  Playing  |    |
    |               +------OnGoalReached()----------|  (Maze    |    |
    |               |                               |  Explore) |    |
    |               v                               +-----------+    |
    |         +-----------+                                          |
    |         |  GameOver |----> 3s ReturnToLobby() --------->------ +
    |         | (Winner   |                                          |
    |         |  Decided) |                                          |
    |         +-----------+                                          |
    +================================================================+
```

---

## 3. Class Interaction

```
    +====================================================================+
    |                  GameInstance (Persist Across Levels)              |
    |                                                                    |
    |  +---------------------+ +------------------+ +----------------+   |
    |  | UMazeGameInstance   | | UUIFlowSubsystem | | USOSManager    |   |
    |  | Network Error Handle| | UI Screen State  | | Session CRUD   |   |
    |  |                     | | Title<->Match    | | Create/Find/   |   |
    |  |                     | |      <->Lobby    | | Join/Destroy   |   |
    |  +---------------------+ +------------------+ +----------------+   |
    +===|===============|==============|===============|=================+
        |               |              |               |
        | NetworkError  |              |               |
        v               v              v               v
    +====================================================================+
    |                         TitleLevel                                 |
    |                                                                    |
    |  +------------------+    +-------------------------+               |
    |  | ATitleGameMode   |    | ATitlePlayerController  |               |
    |  +------------------+    | UI Widget Management    |               |
    |                          +---+-------+-------+-----+               |
    |                              |       |       |                     |
    |                              v       v       v                     |
    |  +----------------+ +----------------+ +----------------+          |
    |  | UTitleWidget   | | UMatchWidget   | | ULobbyWidget   |          |
    |  +----------------+ +-------+--------+ +---+----+-------+          |
    |                             |              |    |                  |
    |                             v              v    v                  |
    |                      +------------+ +---------------------+        |
    |                      |USOSManager | |AMazeLobbyGameState  |        |
    |                      +------------+ | SelectedMazeSize    |        |
    |                                     | bGameStarted        |        |
    |                                     +---------------------+        |
    |                                     +---------------------+        |
    |                                     |AMazeLobbyPlayerState|        |
    |                                     | bIsReady            |        |
    |                                     +---------------------+        |
    |                                     +---------------------+        |
    |                                     |AMazeGameSession     |        |
    |                                     | ExpectedPlayers     |        |
    |                                     +---------------------+        |
    +====================================================================+
        |                                             ^
        | ServerTravel(MazeLevel?listen?MazeSize=X)   | ServerTravel(TitleLevel?listen)
        v                                             |
    +====================================================================+
    |                         MazeLevel                                  |
    |                                                                    |
    |  +-------------------+   +-------------------+                     |
    |  | AMazeGameMode     |-->| AMazeGameState    |                     |
    |  | Game Flow Control |   | Phase, MazeSeed   |                     |
    |  +--------+----------+   | CountdownEndTime  |                     |
    |           |              | WinnerPlayer      |                     |
    |           |              +--------+----------+                     |
    |           |                       |                                |
    |           |              OnRep_Phase                               |
    |           |                       |                                |
    |           v                       v                                |
    |  +-------------------+   +----------------------+                  |
    |  | UMazeGenerator    |   | UMazeCountdownWidget |                  |
    |  | Kruskal Maze Gen  |   | Countdown Display    |                  |
    |  +---+----------+----+   +----------------------+                  |
    |      |          |                                                  |
    |      v          v                                                  |
    |  +---------------+   +-------------------+                         |
    |  |AMazeGoalActor |   |AMazeTargetPoint   |                         |
    |  |Goal Trigger   |-->|Player Spawn Point |                         |
    |  |Sphere         |   +-------------------+                         |
    |  +-------+-------+                                                 |
    |          |                                                         |
    |          | OnGoalReached(WinnerPC)                                 |
    |          v                                                         |
    |  +-------------------+   +-------------------+                     |
    |  | AMazeGameMode     |   | AMazeGameSession  |                     |
    |  | Phase=GameOver    |-->| Login Approval    |                     |
    |  +-------------------+   +-------------------+                     |
    +====================================================================+
```

---

## 4. Network Replication Flow

```
    +=================================+         +=================================+
    |       Server (Authority)        |         |      Client (Replicated)        |
    |                                 |         |                                 |
    |  +------------------+           |         |                                 |
    |  | AMazeGameMode    |           |         |                                 |
    |  +--------+---------+           |         |                                 |
    |           |                     |         |                                 |
    |   GenerateAndSpawnMaze()        |         |                                 |
    |           |                     |         |                                 |
    |           v                     |         |                                 |
    |  +------------------+           |         |                                 |
    |  | UMazeGenerator   |           |         |                                 |
    |  +--------+---------+           |         |                                 |
    |           |                     |         |                                 |
    |    SetMazeData()                |         |                                 |
    |           |                     |         |                                 |
    |           v                     |         |                                 |
    |  +------------------+ OnRep_MazeSeed      |  +------------------+           |
    |  | AMazeGameState   |-----------|-------->|->| AMazeGameState   |           |
    |  |                  |           |         |  +--------+---------+           |
    |  | MazeSeed --------+--Replicate--------->|--> Client Wall Spawn            |
    |  | MazeWidth        |           |         |    (Same Seed = Same Maze)      |
    |  | MazeHeight       |           |         |                                 |
    |  |                  |           |         |                                 |
    |  | Phase -----------+--OnRep_Phase------->|--> UI Switch Per Phase          |
    |  |  Countdown       |           |         |    Countdown: Timer Widget      |
    |  |  Playing         |           |         |    Playing: BGM Start           |
    |  |  GameOver        |           |         |    GameOver: Result Modal       |
    |  |                  |           |         |                                 |
    |  | CountdownEndTime-+--Replicate--------->|--> UMazeCountdownWidget         |
    |  |                  |           |         |    ServerWorldTime Based Sync   |
    |  |                  |           |         |                                 |
    |  | WinnerPlayer ----+--OnRep_MatchResult->|--> BGM Stop                     |
    |  |                  |           |         |    Win/Lose Sound               |
    |  |                  |           |         |    UCommonModalWidget           |
    |  +------------------+           |         |                                 |
    +=================================+         +=================================+
```

---

## 5. Timing Summary

```
    +------------------------------------+----------+-----------------------------------------+
    | Section                            | Default  | Config Location                         |
    +------------------------------------+----------+-----------------------------------------+
    | Lobby -> MazeLevel Travel Delay    | 0.3s     | ULobbyWidget (OnRep propagation wait)   |
    | Player Arrival Timeout             | 30s      | AMazeGameMode::ArrivalTimeoutDuration   |
    | Wall Spawn Interval                | 0.02s    | UMazeGenerator::WallSpawnInterval       |
    | Countdown Duration                 | 10s      | AMazeGameMode::CountdownDuration        |
    | Match End -> Lobby Return          | 3s       | AMazeGameMode::ReturnToLobbyDelay       |
    +------------------------------------+----------+-----------------------------------------+
```

---

## 6. EMazePhase Enum

```
    +---+---------------------+----------------------------------------------+
    | # | Phase               | Description                                  |
    +---+---------------------+----------------------------------------------+
    | 0 | WaitingForPlayers   | MazeLevel entered, waiting for players       |
    | 1 | Countdown           | Maze generated, 10s countdown                |
    | 2 | Playing             | Teleport done, maze exploration              |
    | 3 | GameOver            | Goal reached, winner decided, 3s to return   |
    +---+---------------------+----------------------------------------------+
```
