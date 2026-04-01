# Maze Project — 클래스 구조

## 범례

```
ClassName              C++ 클래스
  → BP_ClassName       C++ 클래스를 상속받은 Blueprint
  → BP_Name → BP_Sub   Blueprint를 상속받은 Blueprint (체인)
(ParentClass) BP_Name  엔진 기본 클래스를 상속받은 순수 Blueprint
```

---

## 클래스 트리

```
Source/Maze/
│
├── GameInstance/
│   └── UMazeGameInstance : UGameInstance
│       - HandleNetworkError (엔진 기본 억제)
│       - OnNetworkFailure 델리게이트 (에러 처리 + TitleLevel 복귀)
│
├── GameMode/
│   ├── ATitleGameMode : AGameModeBase
│   │   → BP_TitleGameMode                        /Game/Blueprints
│   └── AMazeGameMode : AGameModeBase
│       → BP_MazeGameMode                         /Game/Blueprints
│
├── GameSession/
│   └── AMazeGameSession : AGameSession
│       - ApproveLogin으로 게임 시작 후 Join 차단
│       - ExpectedPlayers 세션 메타데이터 관리
│
├── GameState/
│   ├── AMazeGameState : AGameStateBase
│   │   → BP_MazeGameState                        /Game/Blueprints
│   │   - MazeSeed/Width/Height 리플리케이션
│   │   - OnRep_MazeSeed → 클라이언트 벽 스폰
│   └── AMazeLobbyGameState : AGameStateBase
│       - MazeSize, GameStarted, PlayerList 이벤트
│
├── PlayerController/
│   ├── ATitlePlayerController : APlayerController
│   │   → BP_TitlePlayerController                /Game/Blueprints
│   │   - UI Flow 관리 (Title/Match/Lobby 화면 전환)
│   └── AMazePlayerController : APlayerController
│       → BP_MazePlayerController                 /Game/Blueprints
│       - 게임 입력, ESC 볼륨 팝업, ExitToTitle
│
├── PlayerState/
│   └── AMazeLobbyPlayerState : APlayerState
│       - Ready 상태 리플리케이션, Server RPC
│
├── Character/
│   ├── AMazeCharacter : ACharacter
│   │   → BP_MazeCharacter                        /Game/Blueprints
│   │   - ASC, 무기, 플레이어 색상
│   ├── ABotCharacter : AMazeCharacter
│   │   → BP_BotCharacter                         /Game/Blueprints
│   │   - AI 자동 점유, RVO, 입력 차단
│   └── UMazeAnimInstance : UAnimInstance
│       - 스턴 상태를 ASC에서 읽어 애니메이션 반영
│
├── AI/
│   ├── ABotAIController : AAIController
│   │   → BP_BotAIController                      /Game/Blueprints
│   │   - StateTree + AIPerception 기반
│   └── StateTree/
│       ├── USTT_BotPatrol        # 순찰 태스크
│       ├── USTT_BotCombat        # 전투 태스크
│       ├── USTT_BotStun          # 스턴 태스크
│       ├── USTC_BotHasStunTag    # 조건: Stun 태그
│       └── USTC_BotSeesPlayer    # 조건: Perception 감지
│
├── Actor/
│   ├── AMazeGoalActor : AActor
│   │   → BP_GoalActor                            /Game/Blueprints
│   │   - Overlap → 승리 판정
│   ├── AMazeTargetPoint : ATargetPoint
│   │   - 플레이어 스폰 위치 마커
│   └── (AActor) BP_MazeWall                      /Game/Blueprints
│       - 미로 벽 (순수 BP, 메시 + 콜리전)
│
├── GAS/
│   ├── UMazeGameplayTags
│   │   - Native GameplayTag 정의
│   ├── UGA_MazeAttack : UGameplayAbility
│   │   → BP_GA_MazeAttack                        /Game/GAS/GA
│   │   - 몽타주 + SweepTrace + Stun/무적 GE 적용
│   ├── UAN_MazeAttackHitNotify : UAnimNotify
│   │   - 히트 윈도우 → GAS 이벤트 전송
│   ├── AGCA_ElectricTrail : AGameplayCueNotify_Actor
│   │   → BP_GCA_ElectricTrail                    /Game/GAS/GC
│   │   - 스턴 시 Niagara 전기 이펙트
│   ├── (AnimNotify_GameplayCue)
│   │   AN_GC_SlashSound                          /Game/GAS/GC
│   ├── (AnimNotify_GameplayCueState)
│   │   ANS_GC_ElectricTrail                      /Game/GAS/GC
│   ├── (GameplayCueNotify_Static)
│   │   BP_GCS_HitSound                           /Game/GAS/GC
│   ├── (GameplayCueNotify_Static)
│   │   BP_GCS_SlashSound                         /Game/GAS/GC
│   ├── (GameplayEffect)
│   │   GE_Hit                                    /Game/GAS/GE
│   ├── (GameplayEffect)
│   │   GE_Invincibility                          /Game/GAS/GE
│   └── (GameplayEffect)
│       GE_Stun                                   /Game/GAS/GE
│
├── Helper/
│   └── UMazeGenerator
│       - Randomized Kruskal + Union-Find, 벽/골/봇 스폰
│
├── OnlineSubsystem/
│   └── USOSManager : UGameInstanceSubsystem
│       - 세션 Create/Find/Join/Destroy + 재시도 로직
│
├── Settings/
│   ├── UMazeLevelSettings : UDeveloperSettings
│   │   - 레벨 경로 설정 (Config=Game)
│   └── UMazeUserSettings : UGameUserSettings
│       - 볼륨 설정 (Config 자동 저장)
│
├── Tests/
│   └── BotCharacterTest
│       - 봇 AI 자동화 테스트
│
└── UI/
    ├── UUIFlowSubsystem : UGameInstanceSubsystem
    │   - 화면 전환 상태머신 (Title/Match/Lobby + PendingError)
    ├── UTitleWidget : UUserWidget
    │   → WBP_TitleWidget                         /Game/UI
    ├── UMatchWidget : UUserWidget
    │   → WBP_MatchWidget                         /Game/UI
    ├── ULobbyWidget : UUserWidget
    │   → WBP_LobbyWidget                         /Game/UI
    ├── UAudioSettingsWidget : UUserWidget
    │   → WBP_AudioSettings                       /Game/UI
    ├── UCommonModalWidget : UUserWidget
    │   → WBP_CommonModal                         /Game/UI
    │     → WBP_GameResultModal                   /Game/UI
    ├── ULoadingOverlayWidget : UUserWidget
    │   → WBP_LoadingOverlay                      /Game/UI
    ├── UMazeCountdownWidget : UUserWidget
    │   → WBP_MazeCountdown                       /Game/UI
    ├── ULobbyPlayerEntryItem : UObject
    │   - 로비 플레이어 리스트 데이터
    ├── ULobbySearchResultItem : UObject
    │   - 세션 검색 결과 데이터
    ├── (UUserWidget)
    │   WBP_LobbyPlayerRow                        /Game/UI
    │   - 로비 플레이어 행 (순수 BP 위젯)
    └── (UUserWidget)
        WBP_LobbySearchResultRow                  /Game/UI
        - 세션 검색 결과 행 (순수 BP 위젯)
```

---

## 상속 관계 요약

### C++ → Blueprint 상속 체인

| C++ 클래스 | Blueprint | 경로 |
|-----------|-----------|------|
| MazeGameMode | BP_MazeGameMode | /Game/Blueprints |
| TitleGameMode | BP_TitleGameMode | /Game/Blueprints |
| MazeGameState | BP_MazeGameState | /Game/Blueprints |
| MazePlayerController | BP_MazePlayerController | /Game/Blueprints |
| TitlePlayerController | BP_TitlePlayerController | /Game/Blueprints |
| MazeCharacter | BP_MazeCharacter | /Game/Blueprints |
| BotCharacter | BP_BotCharacter | /Game/Blueprints |
| BotAIController | BP_BotAIController | /Game/Blueprints |
| MazeGoalActor | BP_GoalActor | /Game/Blueprints |
| GA_MazeAttack | BP_GA_MazeAttack | /Game/GAS/GA |
| GCA_ElectricTrail | BP_GCA_ElectricTrail | /Game/GAS/GC |
| CommonModalWidget | WBP_CommonModal | /Game/UI |
| TitleWidget | WBP_TitleWidget | /Game/UI |
| MatchWidget | WBP_MatchWidget | /Game/UI |
| LobbyWidget | WBP_LobbyWidget | /Game/UI |
| AudioSettingsWidget | WBP_AudioSettings | /Game/UI |
| LoadingOverlayWidget | WBP_LoadingOverlay | /Game/UI |
| MazeCountdownWidget | WBP_MazeCountdown | /Game/UI |

### Blueprint → Blueprint 상속 체인

| 부모 Blueprint | 자식 Blueprint | 경로 |
|---------------|---------------|------|
| WBP_CommonModal | WBP_GameResultModal | /Game/UI |

### 순수 Blueprint (엔진 클래스 직접 상속)

| 엔진 부모 클래스 | Blueprint | 경로 |
|----------------|-----------|------|
| AActor | BP_MazeWall | /Game/Blueprints |
| AnimNotify_GameplayCue | AN_GC_SlashSound | /Game/GAS/GC |
| AnimNotify_GameplayCueState | ANS_GC_ElectricTrail | /Game/GAS/GC |
| GameplayCueNotify_Static | BP_GCS_HitSound | /Game/GAS/GC |
| GameplayCueNotify_Static | BP_GCS_SlashSound | /Game/GAS/GC |
| GameplayEffect | GE_Hit | /Game/GAS/GE |
| GameplayEffect | GE_Invincibility | /Game/GAS/GE |
| GameplayEffect | GE_Stun | /Game/GAS/GE |
| UUserWidget | WBP_LobbyPlayerRow | /Game/UI |
| UUserWidget | WBP_LobbySearchResultRow | /Game/UI |

---

## GameInstanceSubsystem 구조

```
UMazeGameInstance (GameInstance)
├── USOSManager (GameInstanceSubsystem)
│   - 세션 생명주기 (Create/Find/Join/Destroy)
│   - Pending 재시도 시스템
└── UUIFlowSubsystem (GameInstanceSubsystem)
    - 화면 전환 상태머신 (Title ↔ Match ↔ Lobby)
    - PendingError 패턴
```
