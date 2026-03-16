# Maze — 멀티플레이어 미로 탈출 게임

Unreal Engine 5.6 기반의 멀티플레이어 미로 탈출 게임입니다.
Listen Server 아키텍처로 LAN 및 Steam 환경에서 동작합니다.

---

## 기술 스택

| 구분 | 기술 |
|------|------|
| **엔진** | Unreal Engine 5.6 |
| **언어** | C++ |
| **네트워크** | Listen Server (Non-Seamless Travel) |
| **매치메이킹** | OnlineSubsystem (Null / Steam) |
| **미로 생성** | Kruskal 알고리즘 (시드 기반) |
| **AI** | Behavior Tree + GAS (Gameplay Ability System) |

---

## 아키텍처

### 레벨 구조

```
TitleLevel (매치메이킹)          MazeLevel (게임플레이)
├─ TitlePlayerController        ├─ MazePlayerController
├─ TitleGameMode                ├─ MazeGameMode
├─ MazeLobbyGameState           ├─ MazeGameState
└─ UI: Title → Match → Lobby   └─ MazeGenerator + Bot AI
```

Non-Seamless `ServerTravel`로 레벨 전환 시 모든 GameFramework 클래스가 교체됩니다.

### 네트워크

- **세션 관리**: `USOSManager` (GameInstanceSubsystem) — 레벨 전환에도 유지
- **미로 동기화**: 서버가 시드만 리플리케이트 → 클라이언트가 동일한 미로를 로컬 스폰
  - 네트워크 비용: O(벽 × 플레이어) → O(플레이어)
- **로비 UI**: 이벤트 기반 갱신 (OnRep → 델리게이트 → UI)

### 디렉토리 구조

```
Source/Maze/
├─ Actor/              # 게임플레이 액터 (벽, 골, 아이템 등)
├─ AI/                 # Bot Behavior Tree, AI Controller
├─ Character/          # 플레이어 캐릭터
├─ GameMode/           # TitleGameMode, MazeGameMode
├─ GameState/          # MazeLobbyGameState, MazeGameState
├─ GAS/                # Gameplay Ability System (어빌리티, 이펙트)
├─ Helper/             # MazeGenerator (Kruskal 알고리즘)
├─ OnlineSubsystem/    # SOSManager (세션 생성/검색/참가/파괴)
├─ PlayerController/   # Title/Maze PlayerController
├─ PlayerState/        # MazeLobbyPlayerState, MazePlayerState
├─ Settings/           # 레벨 경로 등 프로젝트 설정
└─ UI/                 # 위젯 (Title, Match, Lobby, Loading, Modal)
```
