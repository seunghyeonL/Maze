# Maze

- Unreal Engine 5.6 기반의 멀티플레이어 미로 탈출 게임입니다.
- Listen Server 아키텍처로 LAN 및 Steam 환경에서 동작합니다.

## 기술 스택

| 구분 | 기술 |
|------|------|
| **엔진** | Unreal Engine 5.6 |
| **언어** | C++ |
| **네트워크** | Listen Server (Non-Seamless Travel) |
| **매치메이킹** | OnlineSubsystem (Null / Steam) |
| **미로 생성** | Kruskal 알고리즘 (시드 기반) |
| **AI** | StateTree + GAS (Gameplay Ability System) |


## 아키텍처

### 레벨 구조


```
TitleLevel (매치메이킹)          MazeLevel (게임플레이)
├─ TitlePlayerController        ├─ MazePlayerController
├─ TitleGameMode                ├─ MazeGameMode
├─ MazeLobbyGameState           ├─ MazeGameState
└─ UI: Title → Match → Lobby   └─ MazeGenerator + Bot AI
```

- Non-Seamless `ServerTravel`로 레벨 전환 시 모든 GameFramework 클래스가 교체됩니다.

### GameInstance


```
UMazeGameInstance
├─ USOSManager (GameInstanceSubsystem)
│   세션 생명주기 관리 (Create / Find / Join / Destroy)
├─ UUIFlowSubsystem (GameInstanceSubsystem)
│   UI 화면 전환 상태머신 (Title ↔ Match ↔ Lobby)
└─ HandleNetworkError
    Travel 중 네트워크 실패 시 안전한 복귀 처리
```

### 디렉토리 구조


```
Source/Maze/
├─ Actor/              # 게임플레이 액터 (벽, 골, 스폰 포인트)
├─ AI/                 # Bot StateTree, AI Controller
├─ Character/          # 플레이어 / 봇 캐릭터, AnimInstance
├─ GameInstance/       # 네트워크 실패 핸들링
├─ GameMode/           # TitleGameMode, MazeGameMode
├─ GameState/          # MazeLobbyGameState, MazeGameState
├─ GAS/                # Gameplay Ability System (어빌리티, 이펙트, 큐)
├─ Helper/             # MazeGenerator (Kruskal 알고리즘)
├─ OnlineSubsystem/    # SOSManager (세션 관리)
├─ PlayerController/   # Title / Maze PlayerController
├─ PlayerState/        # MazeLobbyPlayerState
├─ Settings/           # 레벨 경로, 사용자 볼륨 설정
└─ UI/                 # 위젯 (Title, Match, Lobby, Loading, Modal)
```

### 문서

- [전체 클래스 구조](Docs/Class_Structure.md)
- [게임플로우 다이어그램](Docs/GameFlow_Diagram_Ascii.md)
