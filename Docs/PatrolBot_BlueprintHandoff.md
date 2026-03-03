# Patrol Bot — Blueprint Handoff

This document tells you exactly what to create and configure in the Unreal Editor to wire up the C++ patrol bot system. All logic lives in C++; your job here is to create the Blueprint shells, assign references, and build the StateTree.

---

## 1. Assets to Create

- [ ] **BP Class**: `BP_BotCharacter` (Parent: `ABotCharacter`)
  - Location: `/Game/AI/`
  - Purpose: The bot pawn. Holds the ability list and mesh setup.

- [ ] **BP Class**: `BP_BotAIController` (Parent: `ABotAIController`)
  - Location: `/Game/AI/`
  - Purpose: Runs the StateTree and owns the Perception component.

- [ ] **StateTree**: `ST_BotBrain`
  - Location: `/Game/AI/`
  - Purpose: Drives Patrol / Combat / Stun behavior via three prioritized states.

---

## 2. Editor Steps

### 2.1 Create `BP_BotCharacter`

1. Content Browser → **Add → Blueprint Class** → search for `ABotCharacter` → name it `BP_BotCharacter`, save to `/Game/AI/`.
2. Open `BP_BotCharacter` → **Class Defaults**:
   - **Skeletal Mesh**: assign the same mesh used by your player character.
   - **Anim Blueprint**: assign the same AnimBP used by your player character.
3. Still in Class Defaults, find **Default Abilities** (array):
   - Add one entry: `GA_MazeAttack`.
4. Compile and Save.

### 2.2 Create `BP_BotAIController`

1. Content Browser → **Add → Blueprint Class** → search for `ABotAIController` → name it `BP_BotAIController`, save to `/Game/AI/`.
2. Open `BP_BotAIController` → **Components** panel → select **StateTreeComponent**.
3. In the Details panel, set **StateTree** = `ST_BotBrain` (you'll create this next).
4. Compile and Save.

### 2.3 Create `ST_BotBrain` (StateTree)

1. Content Browser → **Add → Artificial Intelligence → StateTree** → name it `ST_BotBrain`, save to `/Game/AI/`.
2. Open `ST_BotBrain`. In the **Schema** section at the top:
   - **AIController Class** = `BP_BotAIController`
   - **Context Actor Class** = `BP_BotCharacter`
3. Build three states in priority order (top = highest priority):

---

#### State 1: Stun (highest priority)

- Click **+ Add State**, name it `Stun`.
- **Enter Conditions** tab → **+** → pick `STC_BotHasStunTag`:
  - `Actor` = bind to **Context Actor**
- **Tasks** tab → **+** → pick `STT_BotStun`:
  - `AIController` = bind to **Context AIController**
- **Transitions** tab → **+**:
  - Trigger: **On Event** `StateTree.Event.StunEnded`
  - Transition To: `Patrol`

---

#### State 2: Combat

- Click **+ Add State**, name it `Combat`.
- **Enter Conditions** tab → **+** → pick `STC_BotSeesPlayer`:
  - `AIController` = bind to **Context AIController**
- **Tasks** tab → **+** → pick `STT_BotCombat`:
  - `AIController` = bind to **Context AIController**
  - `Actor` = bind to **Context Actor**
  - `TargetPlayer` = bind to `STC_BotSeesPlayer.SeenPlayer`
  - `AttackAbilityClass` = `GA_MazeAttack`
- **Transitions** tab → **+**:
  - Trigger: **On Event** `StateTree.Event.Stunned`
  - Transition To: `Stun`

---

#### State 3: Patrol (default, lowest priority)

- Click **+ Add State**, name it `Patrol`.
- No enter conditions (this is the fallback state).
- **Tasks** tab → **+** → pick `STT_BotPatrol`:
  - `AIController` = bind to **Context AIController**
  - `Actor` = bind to **Context Actor**
  - `CellSize` = set to your maze's cell size (default: `400`)
- **Transitions** tab → **+** (add two):
  1. Trigger: **On Event** `StateTree.Event.PlayerSpotted` → Transition To: `Combat`
  2. Trigger: **On Event** `StateTree.Event.Stunned` → Transition To: `Stun`

4. Compile and Save `ST_BotBrain`.

---

### 2.4 NavMesh Setup

1. In your level, place a **Nav Mesh Bounds Volume** (search in Place Actors panel).
2. Scale it to cover the entire maze floor area. Bots won't move without this.
3. Press **P** in the viewport to visualize the nav mesh (green = navigable).

### 2.5 GameMode / Spawn Setup

1. Open your GameMode Blueprint (or Level Blueprint).
2. On **BeginPlay** (or wherever you call maze generation), call **GenerateMaze**:
   - `BotClass` = `BP_BotCharacter`
   - `BotCount` = `4`
3. Compile and Save.

---

## 3. Wiring / References

- [ ] `BP_BotCharacter.DefaultAbilities[0]` = `GA_MazeAttack`
- [ ] `BP_BotAIController.StateTreeComponent.StateTree` = `ST_BotBrain`
- [ ] `STT_BotCombat.AttackAbilityClass` = `GA_MazeAttack`
- [ ] `STT_BotPatrol.CellSize` = match your maze's `CellSize` (default `400`)

**Gameplay Tags** — verify all three exist in **Project Settings → GameplayTags**:
- [ ] `StateTree.Event.Stunned`
- [ ] `StateTree.Event.StunEnded`
- [ ] `StateTree.Event.PlayerSpotted`

If any are missing, add them manually. The GAS-to-StateTree bridge and Perception bridge both fire these tags; if they're not registered the events will silently fail.

---

## 4. Blueprint Graph Steps

No custom graph work is needed. All behavior runs in C++.

The only graph step is in your **GameMode BP** or **Level Blueprint**:

- [ ] On `BeginPlay` → call `GenerateMaze` node:
  - `BotClass` = `BP_BotCharacter`
  - `BotCount` = `4`

That's it. The StateTree, Perception callbacks, and GAS integration are all wired in C++.

---

## 5. Quick Verification Checklist (PIE)

- [ ] Build succeeded in Rider with no errors
- [ ] PIE starts without warnings or errors in the Output Log
- [ ] 4 bots spawn at maze corners on play start
- [ ] Bots move around the maze (Patrol state active)
- [ ] Walk close to a bot → bot turns and chases you (Combat state activates)
- [ ] Bot closes to attack range → `GA_MazeAttack` activates (check log or hit reaction)
- [ ] Apply stun tag to a bot → bot stops moving immediately (Stun state activates)
- [ ] Stun expires → bot resumes patrol (StunEnded event fires, back to Patrol)
- [ ] **Networking**: Listen Server + one Client:
  - [ ] Bots visible on both server and client
  - [ ] Bot movement replicates to client
  - [ ] Attack and stun effects replicate correctly

---

## 6. Debug Notes

**StateTree asset not assigned**
`BP_BotAIController → StateTreeComponent → StateTree` must point to `ST_BotBrain`. If it's empty, the bot spawns but does nothing.

**NavMesh not covering the maze**
Bots will stand still. Press **P** in the viewport to check coverage. Scale the `NavMeshBoundsVolume` until the entire floor is green.

**`GA_MazeAttack` not in `DefaultAbilities`**
The Combat task calls `AttackAbilityClass` via GAS. If the ability isn't granted at spawn, the attack silently fails. Check `BP_BotCharacter → Class Defaults → DefaultAbilities`.

**StateTree events not registered**
If `StateTree.Event.Stunned`, `StateTree.Event.StunEnded`, or `StateTree.Event.PlayerSpotted` are missing from Project Settings → GameplayTags, the state transitions never fire. Add them manually if needed.

**`BotClass` not set in `GenerateMaze()`**
Bots won't spawn at all. Make sure the `BotClass` pin on the `GenerateMaze` node is set to `BP_BotCharacter`, not left as `None`.

**Bots detect each other**
They shouldn't. Bot team ID is `1`, player team ID is `0`, and the Perception component is configured for enemies only. If bots are chasing each other, check that `BP_BotCharacter` doesn't accidentally inherit a team ID of `0`.

**StateTree schema mismatch**
If you see a schema error when opening `ST_BotBrain`, make sure the AIController Class and Context Actor Class in the schema match `BP_BotAIController` and `BP_BotCharacter` exactly (not the C++ base classes).
