# Maze — UE5 멀티플레이어 미로 탈출 게임

> **기업 제출용 포트폴리오 문서**

---

## 1. 프로젝트 개요

**4인 멀티플레이어 미로 탈출 게임.** 서버 권위(Server-Authoritative) 아키텍처 기반으로, 플레이어들이 절차적으로 생성된 미로에서 탈출을 경쟁하며 GAS 기반 전투와 StateTree AI 봇을 상대한다.

### 기술 스택

| 분류 | 기술 |
|------|------|
| 엔진 | Unreal Engine 5.6 |
| 언어 | C++ |
| 멀티플레이어 | OnlineSubsystem (Steam / LAN Null) |
| 전투 | Gameplay Ability System (GAS) |
| AI | StateTree + AIPerception |
| UI | UMG + GameInstanceSubsystem |
| 입력 | Enhanced Input |
| 이펙트 | Niagara |
| 설정 영속화 | UGameUserSettings (Config INI) |

### 핵심 키워드
`멀티플레이어` · `Replication` · `GAS` · `StateTree AI` · `절차적 미로 생성` · `ServerTravel` · `Subsystem Architecture`

---

## 2. 프로젝트 구조

### 디렉토리 구조

```
Source/Maze/
├── Actor/           (2)  — 목표 트리거(MazeGoalActor), 스폰 포인트(MazeTargetPoint)
├── AI/              (5)  — BotAIController + StateTree 조건/태스크 4종
├── Character/       (3)  — MazeCharacter(플레이어), BotCharacter, MazeAnimInstance
├── GameMode/        (2)  — TitleGameMode, MazeGameMode
├── GameState/       (2)  — MazeLobbyGameState, MazeGameState
├── GAS/             (4)  — GameplayTags, GA_MazeAttack, AN_MazeAttackHitNotify, GCNA_ElectricTrail
├── Helper/          (1)  — MazeGenerator (Kruskal's Union-Find)
├── OnlineSubsystem/ (1)  — SOSManager
├── PlayerController/(2)  — TitlePlayerController, MazePlayerController
├── PlayerState/     (1)  — MazeLobbyPlayerState
├── Settings/        (2)  — MazeUserSettings, MazeLevelSettings
├── UI/             (10)  — 위젯 10종 + UIFlowSubsystem
└── Tests/           (1)  — 봇 유닛 테스트
```

### 핵심 설계 패턴

**① GameInstanceSubsystem 기반 서비스 계층**

레벨 전환(ServerTravel)에도 살아남아야 하는 상태를 Subsystem으로 분리했다.

| Subsystem | 역할 |
|-----------|------|
| `USOSManager` | 세션 생성/탐색/참가/해제 전담 |
| `UUIFlowSubsystem` | Title/Match/Lobby 화면 상태 머신, 에러 메시지 큐 |

**② Phase 기반 게임 흐름**

```cpp
enum class EMazePhase : uint8 {
    WaitingForPlayers,  // 플레이어 수집 대기
    Countdown,          // 카운트다운
    Playing,            // 게임 진행 중
    GameOver            // 종료
};
```

`AMazeGameState`에 Replicated로 관리. `OnRep_Phase()` 콜백이 각 클라이언트의 UI/오디오를 자동 갱신.

**③ GAS 통합 아키텍처**

```
플레이어 입력
  → GA_MazeAttack (GameplayAbility) 활성화
    → PlayMontageAndWait
      → AN_MazeAttackHitNotify (AnimNotify) 발화
        → SphereTrace 히트 감지
          → GameplayEffect(Stun) 적용
            → GCNA_ElectricTrail (GameplayCue) Niagara 이펙트
```

### 전체 게임 흐름

```
[Title Screen]
  └─ Create Lobby / Find Lobby
      ↓ ServerTravel / ClientTravel
[Session Browser (Match Screen)]
  └─ Join Session
      ↓ ClientTravel
[Lobby]
  └─ 플레이어 Ready + Host Start
      ↓ ServerTravel
[Maze Match]
  ├─ 절차적 미로 생성
  ├─ 플레이어 전투 (GAS)
  ├─ AI 봇 순찰/공격 (StateTree)
  └─ 첫 골 도달 → GameOver
      ↓ ServerTravel
[Lobby] (반복)
```

---

## 3. 핵심 기능 구현

### 3-1. 절차적 미로 생성 (`UMazeGenerator`)

**Kruskal's Algorithm + Union-Find** 로 매 매치마다 다른 미로를 생성한다.

```cpp
// FCell: 미로 그리드의 단위 셀
struct FCell {
    bool RightWall;
    bool DownWall;
    int32 PlayerStartNum;  // -1: 일반 셀, 0~3: 플레이어 스폰
    bool IsGoal;
};
```

- Union-Find에 **경로 압축**을 적용해 연결 여부를 O(α(n)) 수준으로 판별
- 매 매치마다 새 미로 생성 → 리플레이성 확보
- 생성 완료 후 **벽 액터 / 골 액터 / 스폰 포인트 / 봇** 자동 배치

---

### 3-2. 멀티플레이어 시스템 (`USOSManager`) ★

Steam OnlineSubsystem을 추상화한 GameInstanceSubsystem. 세션 전체 생명주기를 담당한다.

**세션 흐름:**

| 단계 | 메서드 | 설명 |
|------|--------|------|
| 호스트 생성 | `CreateSession()` | 세션 등록, ServerTravel 트리거 |
| 클라이언트 탐색 | `FindSessions()` | 세션 목록 조회, UI에 델리게이트 전달 |
| 참가 | `JoinSessionByIndex()` | 내부적으로 ClientTravel 실행 |
| 정리 | `DestroySession()` | 세션 해제 + 재시도 큐(`bPendingCreateAfterDestroy`) |

**비동기 처리**: 모든 결과는 `BlueprintAssignable` 델리게이트(`OnSessionCreated`, `OnSessionsFound` 등)로 UI에 전달.

**Replication 전략**: `GameState`(Phase, Winner), `PlayerState`(bIsReady)에 복제. `ForceNetUpdate()` 호출로 타임크리티컬 변경(카운트다운 시작) 즉시 전파.

---

### 3-3. GAS 전투 시스템 (`UGA_MazeAttack`)

```
공격 입력
→ GA_MazeAttack::ActivateAbility()
  → PlayMontageAndWait (몽타주 재생)
    → AN_MazeAttackHitNotify::Notify() (히트 판정 윈도우)
      → SendGameplayEventToActor → OnAttackHitEvent()
        → SphereTrace (반경 내 대상 탐색)
          → ApplyGameplayEffectToTarget (Stun GE)
          → ApplyGameplayEffectToSelf (Invincibility GE)
```

- **중복 히트 방지**: `LastProcessedAttackNotifyId_Server`로 동일 Notify 중복 처리 차단
- **실행 중 스턴 인터럽트**: `WaitGameplayTagAdded(State.Debuff.Stun)` → `EndAbilityCleanly()` 즉시 중단
- **GameplayCue**: `GCNA_ElectricTrail` — 공격자 무기 소켓에 Niagara 이펙트 부착

---

### 3-4. AI 봇 시스템 (`ABotAIController` + StateTree)

| 상태 | 태스크 클래스 | 동작 |
|------|---------------|------|
| Patrol | `FSTT_BotPatrol` | 랜덤 셀 중심 이동 (4방향 레이캐스트로 벽 회피) |
| Combat | `FSTT_BotCombat` | Chase → PreAttack → Attacking → Cooldown FSM |
| Stun | `FSTT_BotStun` | `State.Debuff.Stun` 태그 해제까지 정지 |

- **전환 조건**: `FSTC_BotSeesPlayer` (AIPerception 시야), `FSTC_BotHasStunTag` (GAS 태그)
- **코드 재사용**: 봇이 플레이어와 동일한 `GA_MazeAttack` 어빌리티 사용
- **팀 구분**: `IGenericTeamAgentInterface` → Bot=1, Player=0 → 서로를 지각 대상으로 인식

---

### 3-5. UI 시스템 설계 (`UUIFlowSubsystem`) ★

화면 전환을 **중앙 집중식 상태 머신**으로 관리한다.

```cpp
enum class EUIFlowScreen : uint8 { Title, Match, Lobby };

// 화면 전환 예시
UIFlowSubsystem->SetScreenMatch();          // → OnScreenChanged 브로드캐스트
// → TitlePlayerController::HandleScreenChanged()
// → RefreshUI() → CreateWidget<UMatchWidget>()
```

**주요 특징:**
- `GameInstanceSubsystem`이므로 ServerTravel 후에도 화면 상태 유지
- **에러 메시지 큐**: `SetPendingError()` → 레벨 전환 후 다음 화면의 모달로 자동 표시
- **Delegate 라이프사이클**: `NativeConstruct`에서 Bind, `NativeDestruct`에서 반드시 Unbind
- **CommonModalWidget**: Alert/Confirm 재사용 모달로 일관된 에러 처리 UX

---

### 3-6. 오디오 설정 시스템

- `UMazeUserSettings` (UGameUserSettings 확장): Master/BGM/SFX 볼륨을 `GameUserSettings.ini`에 영속화
- SoundMix/SoundClass 계층으로 3채널 독립 제어
- **레벨별 분리**: TitleLevel에서는 SoundMix 미적용(ServerTravel 크래시 원인 회피), MazeLevel 진입 시 `MazePlayerController::InitializeAudio()`가 저장된 볼륨값 적용

---

## 4. 트러블슈팅

### 🔴 사례 1: Client → ListenServer GAS 어빌리티 활성화 실패

**문제**: 클라이언트에서 공격 입력 시 서버에서 어빌리티 활성화가 실패.

**원인 분석**:
- UE Replication은 `OnRep_` 콜백 실행 순서를 보장하지 않는다
- `OnRep_PlayerState`가 `OnRep_Controller`보다 먼저 실행되면 `InitAbilityActorInfo` 시점에 `PlayerController = nullptr`
- LocalPredicted 어빌리티는 유효한 PlayerController가 필수

**해결**:
```cpp
// MazeCharacter.cpp
void AMazeCharacter::OnRep_Controller()
{
    Super::OnRep_Controller();
    // Controller가 늦게 도착해도 재시도
    if (APlayerState* PS = GetPlayerState())
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
        {
            ASC->InitAbilityActorInfo(PS, this);
        }
    }
}
```

**교훈**: `OnRep_PlayerState`와 `OnRep_Controller` 양쪽에서 `InitAbilityActorInfo`를 호출해 먼저 도착하는 쪽이 초기화를 완료하도록 설계해야 한다.

---

### 🔴 사례 2: 서버에서 AnimNotify 미발화 → Hit Detection 실패

**문제**: 클라이언트 캐릭터의 공격 몽타주 AnimNotify가 서버에서 실행되지 않아 히트 판정 불가.

**원인 분석**:
- UE5 기본값 `EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered`
- 서버에는 렌더러가 없으므로 Skeletal Mesh 애니메이션 틱 자체가 생략됨
- AnimNotify는 애니메이션 틱 중에만 발화 → 서버에서 완전히 무시

**해결**:
```cpp
// MazeCharacter.cpp — Constructor
GetMesh()->VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
```

`AlwaysTickPoseAndRefreshBones` 대신 **몽타주만 틱**하는 옵션을 선택해 성능 영향을 최소화.

**교훈**: 멀티플레이어에서 서버 측 캐릭터 애니메이션 틱 정책은 반드시 명시적으로 설정해야 한다.

---

### 🔴 사례 3: Audio + ServerTravel 크래시 (RecursiveApplyAdjuster)

**문제**: TitleLevel에서 CreateLobby 클릭 → ServerTravel 시 치명적 크래시.

```
Fatal error: [UE-63471] RecursiveApplyAdjuster
  → FAudioDevice::SetSoundMixClassOverride
  → FAudioDevice::ApplyAudioSettings
```

**원인 분석**:
- `SetBaseSoundMix()` / `SetSoundMixClassOverride()`는 FAudioDevice(엔진 수명)에 비동기로 상태를 등록
- Non-seamless ServerTravel 시 이전 레벨 객체들이 GC되지만, 오디오 스레드는 여전히 해당 포인터 처리 중
- GC 완료 전 오디오 스레드가 해제된 포인터에 접근 → 댕글링 포인터 크래시

**해결**: TitleLevel에서 SoundMix 조작을 완전히 제거. 오디오 설정값은 `MazeUserSettings`에만 저장되고, MazeLevel 진입 시 `MazePlayerController::InitializeAudio()`가 적용.

**교훈**: `FAudioDevice`는 엔진 생명주기의 전역 상태이며, Non-seamless ServerTravel 전후로 오디오 상태 정리가 필수다.

---

### 🟡 사례 4: Replication 순서 불일치 → BGM + 결과음 중첩

**문제**: GameOver 시 BGM이 멈추지 않고 결과음과 동시 재생.

**원인 분석**:
```
// 문제가 되는 설계
OnRep_Phase(GameOver) → BGM 정지
OnRep_MatchResult()   → 결과음 재생
// 두 OnRep이 같은 프레임에 다른 순서로 도착할 수 있음
```

**해결**: BGM 정지와 결과음 재생을 `OnRep_MatchResult` 단일 콜백에 통합.

**교훈**: 동일 프레임에 변경되는 복수의 Replicated 속성이 의존 관계에 있다면, 단일 콜백에서 함께 처리해야 순서를 보장할 수 있다.

---

### 🟡 사례 5: 공격 중 스턴 무시 문제

**문제**: 공격 몽타주 실행 중 스턴을 받아도 공격이 끝까지 완료됨.

**원인 분석**:
- `ActivationBlockedTags`는 **새 활성화만 차단**, 실행 중인 어빌리티에는 효과 없음
- 이미 실행 중인 `GA_MazeAttack`은 스턴 태그와 무관하게 계속 진행

**해결**:
```cpp
// GA_MazeAttack.cpp
void UGA_MazeAttack::ActivateAbility(...)
{
    // 스턴 태그 추가 감시
    UAbilityTask_WaitGameplayTagAdded* StunWatcher =
        UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
            this, Tags.State_Debuff_Stun);
    StunWatcher->Added.AddDynamic(this, &UGA_MazeAttack::OnStunTagAdded);
    StunWatcher->ReadyForActivation();
}

void UGA_MazeAttack::OnStunTagAdded()
{
    EndAbilityCleanly();  // 몽타주 인터럽트 + 어빌리티 즉시 종료
}
```

**교훈**: GAS에서 실행 중인 어빌리티의 상태 변화 감시는 `AbilityTask`를 통해 별도로 구현해야 한다.

---

## 5. 코드 자기평가

### ✅ 잘한 점

**1. 방어적 프로그래밍**
`MazeGenerator`, `SOSManager` 전반에 걸쳐 null 체크와 조기 반환 패턴을 일관되게 적용. 특히 `MazeGenerator::GenerateMaze()`는 WorldContext, World, 필수 클래스들을 순서대로 검증하고 각각 구체적인 에러 로그를 남긴다.

**2. 현대적 UE5 패턴 활용**
GameInstanceSubsystem, GAS Mixed Replication Mode, Enhanced Input System, StateTree, Niagara GameplayCue 등 UE5의 현대적 프레임워크를 프로젝트 규모에 맞게 적극 활용했다.

**3. 명확한 상태 머신 설계**
`EMazePhase` 기반 게임 흐름 + `bGameFlowStarted` / `bMatchEnded` 재진입 가드. `STT_BotCombat`의 Chase → PreAttack → Attacking → Cooldown FSM. 상태 전환 조건이 명시적이어서 레이스 컨디션을 방지한다.

**4. Delegate 라이프사이클 관리**
`NativeConstruct`에서 Bind, `NativeDestruct`에서 반드시 Unbind. `AddDynamic` 전에 항상 `RemoveDynamic` 선행 호출로 중복 바인딩을 방지. `BoundPlayerStates`를 `TWeakObjectPtr`로 추적.

**5. 알고리즘 정확성**
Kruskal's Union-Find를 경로 압축(path compression)까지 포함해 정확히 구현. 단순한 패턴 복사가 아닌 알고리즘 원리를 이해하고 적용.

---

### ⚠️ 아쉬운 점

**1. 매직 넘버 산재**
```cpp
// MazeGenerator.cpp — 플레이어 스폰 위치 하드코딩
const TArray<FIntPoint> Corners = {{0,0}, {0,Width-1}, {Height-1,0}, {Height-1,Width-1}};

// LobbyWidget.cpp — 미로 크기 옵션 하드코딩
ComboBox->AddOption(TEXT("5"));  // {5, 7, 9, 11}
```
→ **개선 방향**: `UPROPERTY(EditDefaultsOnly)` 또는 DataAsset으로 외부화

**2. 코드 중복 (DRY 위반)**
```cpp
// GA_MazeAttack.cpp — GameplayEffect 적용 패턴이 2회 반복
FGameplayEffectContextHandle StunContext = SourceASC->MakeEffectContext();
FGameplayEffectSpecHandle StunSpec = SourceASC->MakeOutgoingSpec(StunEffectClass, ...);
SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data.Get(), TargetASC);
// ... InvincibilityEffectClass에서 동일 패턴 반복
```
→ **개선 방향**: `ApplyEffectToTarget(TSubclassOf<UGameplayEffect>, ...)` 헬퍼 추출

**3. 비동기 콜백 안전성**
```cpp
// SOSManager.cpp — Destructor에서 OnlineSubsystem 델리게이트 핸들 미해제
// 객체 파괴 후 콜백이 실행되면 댕글링 포인터 위험
```
→ **개선 방향**: Destructor에서 모든 `Handle.Reset()`, 람다에 `TWeakObjectPtr` 적용

**4. 모듈 간 결합도**
```cpp
// MatchWidget.cpp, LobbyWidget.cpp — 여러 위젯에서 동일 패턴 반복
SOSManager = GetGameInstance()->GetSubsystem<USOSManager>();
UIFlowSubsystem = GetGameInstance()->GetSubsystem<UUIFlowSubsystem>();
```
→ **개선 방향**: 위젯 베이스 클래스나 인터페이스로 서브시스템 접근을 중앙화

**5. 일부 에지케이스 미처리**
- 어빌리티 실행 중 Avatar 파괴 시 크래시 가능성
- `WinnerPlayer` null 접근 가능성 (직접 참조 시)

→ **개선 방향**: 어빌리티 내 `IsValid(Avatar)` 체크 강화, Weak 참조 활용

---

## 6. 기술적 성장 포인트

### 이 프로젝트를 통해 배운 교훈

**① Replication 순서는 보장되지 않는다**
`OnRep_PlayerState`와 `OnRep_Controller`의 실행 순서 문제로 GAS 초기화 버그를 경험했다. 의존 관계가 있는 상태 변경은 단일 콜백에 통합하거나, 양쪽에서 재시도하는 방어 코드가 필수임을 배웠다.

**② UE5 내부 시스템의 생명주기를 이해해야 한다**
`FAudioDevice`가 엔진 전역 상태임을 모르고 TitleLevel에서 SoundMix를 조작했다가 ServerTravel 크래시를 마주했다. 엔진 내부 시스템(AudioDevice, AnimTick 정책)의 생명주기를 사전에 파악하지 않으면 디버깅 비용이 기하급수적으로 증가한다.

**③ Subsystem은 멀티플레이어 게임의 강력한 도구다**
레벨 전환이 잦은 멀티플레이어 구조에서 GameInstanceSubsystem이 세션 상태와 UI 상태를 안전하게 유지해주는 것을 직접 경험했다. 서비스 계층을 Subsystem으로 설계하면 레벨 전환 관련 버그를 구조적으로 방지할 수 있다.

### 다음 프로젝트 개선 목표

- [ ] 설정값을 처음부터 `UPROPERTY(EditDefaultsOnly)` / DataAsset으로 외부화
- [ ] 비동기 콜백 패턴에서 `TWeakObjectPtr` + Destructor Handle 해제 기본 적용
- [ ] 인터페이스 추상화로 모듈 간 결합도 낮추고 테스트 가능한 구조 설계
- [ ] 반복 패턴을 헬퍼 함수로 즉시 추출 (DRY 원칙 초기부터 적용)

---
