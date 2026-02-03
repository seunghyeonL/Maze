# AGENTS.md

## Project scope
- The Unreal project root is: .
- Only modify files under .

## Editing scope
- Primary edit location: ./Source/Maze/** (C++ code)
- Allowed when required for compilation:
  - ./Source/Maze/Maze.Build.cs
  - ./Maze.uproject (only if module registration is missing)
  - ./Config/** (rare; only if build requires it)
- Do not modify anything outside . without asking.

## Tooling
- Prefer Rider MCP tools for file edits and running builds.
- Prefer Run Configurations; if unavailable, use Rider terminal commands.

## Workflow
- Implement core systems in C++ first, then hand off remaining editor/Blueprint work to the user.
- Expose Blueprint integration points where appropriate:
  - Use UPROPERTY(EditDefaultsOnly / EditAnywhere, BlueprintReadOnly/BlueprintReadWrite) for tunables.
  - Use UFUNCTION(BlueprintCallable) for calls from BP.
  - Use UFUNCTION(BlueprintImplementableEvent / BlueprintNativeEvent) when BP should extend behavior.
  - Avoid hard-coded asset paths; prefer soft references (TSoftObjectPtr/TSoftClassPtr) or Data Assets.
- After C++ changes, always include a **Blueprint Handoff** section with:
  - Assets to create (BP classes, widgets, data assets)
  - Editor steps (what to set and where)
  - Required wiring (references, components, inputs, tags, data tables)
  - Quick verification checklist (PIE/in-editor test)

## Safety
- Do not delete files or run cleanup commands unless explicitly asked.
- Avoid mass refactors; keep changes minimal.
- If modification is required for compilation, proceed and report why. Otherwise ask first.
  - Applies especially to: .uproject / .uplugin / Build.cs / Config

## Reporting
- Always list changed file paths.
- Summarize build command/config used and the result (success/failure + key errors).
- Definition of Done (DoD):
  - Build succeeds
  - Changed files listed
  - Blueprint Handoff included (when relevant)

## Blueprint Handoff (Template)

### 1) Assets to create
- [ ] **BP Class**: `BP_???` (Parent: `???`)
  - Location: `/Game/...`
  - Purpose: (한 줄 요약)
- [ ] **Widget**: `WBP_???` (optional)
  - Location: `/Game/UI/...`
- [ ] **Data Asset**: `DA_???` (Type: `U???DataAsset`) (optional)
  - Location: `/Game/Data/...`
- [ ] **Input**: `IA_???`, `IMC_???` (Enhanced Input) (optional)
  - Location: `/Game/Input/...`
- [ ] **Other**: (AnimBP, Montage, DT, Curve, Niagara, Material 등)

### 2) Editor steps
> “어디를 눌러서 뭘 설정해야 하는지”를 순서대로 적습니다.
1. [ ] Create `BP_???` from `???` (Content Browser → Add → Blueprint Class)
2. [ ] Open `BP_???` → Components:
   - [ ] Add Component: `???`
   - [ ] Set property `???` = `???`
3. [ ] Project Settings → Input:
   - [ ] Add Mapping Context `IMC_???`
   - [ ] Bind Action `IA_???` to (Key/Mouse/Gamepad)
4. [ ] World/Level:
   - [ ] Place actor `BP_???` in level (or set as Default Pawn/GameMode)

### 3) Wiring / References
> C++에서 노출한 연결 포인트를 BP/에디터에서 연결합니다.
- [ ] Assign class references:
  - [ ] `SomeClass` property in `BP_???` = `BP_???`
- [ ] Assign assets / soft references:
  - [ ] `TSoftClassPtr` / `TSoftObjectPtr` fields set to `/Game/...`
- [ ] Bind delegates / events:
  - [ ] On `???` → bind to `???` (BP graph)
- [ ] Gameplay Tags (if used):
  - [ ] Add tags in Project Settings → GameplayTags
  - [ ] Set tag fields on `BP_???` / `DA_???`

### 4) Blueprint Graph steps (if any)
- [ ] In `BP_???` Event Graph:
  - [ ] Implement event `Event ???` (BlueprintImplementableEvent/NativeEvent)
  - [ ] Call `???` (BlueprintCallable) on (BeginPlay / Input / UI callback)
  - [ ] Update UI widget `WBP_???` (set text/progress/etc.)

### 5) Quick verification checklist (PIE)
- [ ] Build succeeded in Rider
- [ ] PIE starts without warnings/errors
- [ ] Repro steps:
  1. [ ] (예: 플레이 시작 → 키 `E` 누름)
  2. [ ] Expected: (예: 로그 출력/이펙트/위젯 변화/상태 전환)
- [ ] Networking (if relevant):
  - [ ] Listen Server + Client에서 동작 확인
  - [ ] Replication 확인(서버에서만 생성/클라에서 반영)

### 6) Debug notes (optional)
- Common pitfalls:
  - [ ] Class not set in defaults / wrong parent class
  - [ ] Mapping Context not added to player
  - [ ] Soft reference not loaded (need async load or Ensure loaded)
  - [ ] Tags not registered