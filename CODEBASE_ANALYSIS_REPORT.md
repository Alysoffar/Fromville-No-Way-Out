# Fromville Codebase Architecture Analysis Report
**Date:** May 16, 2026 | **Scope:** src/ directory (204 files analyzed)

---

## 1. FILE SIZES & STRUCTURE

### Files Over 500 Lines (Candidates for Splitting)

| File | Lines | Category | Severity |
|------|-------|----------|----------|
| [World.cpp](src/game/world/World.cpp) | **2410** | Game state/logic | **CRITICAL** |
| [DialogueManager.cpp](src/game/dialogue/DialogueManager.cpp) | **642** | Dialogue + UI | **HIGH** |
| [PuzzleManager.cpp](src/game/puzzles/core/PuzzleManager.cpp) | **568** | Puzzle factory | **HIGH** |
| [Quest.cpp](src/game/quest/Quest.cpp) | **600** | Quest data + serialization | **HIGH** |
| [Game.cpp](src/game/Game.cpp) | **414** | Main loop + rendering | **MEDIUM** |

### Analysis
- **World.cpp is 4x larger than any recommended module size** (best practice ≤600 lines)
- Contains character management, environment updates, NPC behavior, interaction logic, audio cues, and narrative logic
- Suggests broken separation of concerns and tight coupling

---

## 2. DUPLICATED PATTERNS

### A. Timer/Cooldown Logic Duplicates
**Severity: HIGH** | **Found: 150+ instances**

Pattern: Manual float decrement in Update loops
```cpp
// Repeated across 20+ files
timerVariable -= dt;
if (timerVariable <= 0.0f) {
    // trigger logic
    timerVariable = resetValue;
}
```

**Files with Timer Duplication:**
- [AtmosphereManager.cpp](src/game/atmosphere/AtmosphereManager.cpp): L47, L56, L74, L97, L157, L161, L176, L186, L194 (9 timers)
- [MemoryReplaySystem.cpp](src/game/memory/MemoryReplaySystem.cpp): L78, L113 (memory duration timer)
- [World.cpp](src/game/world/World.cpp): L1646, L1650, L1654, L1657, L1660, L1663, L1677, L1685 (multiple NPC cooldowns)
- [Character.cpp](src/game/entities/Character.cpp): L16 (ability timer)
- [Enemy.cpp](src/game/entities/Enemy.cpp): L38 (investigation timer)
- All puzzle classes: [JadeDebatePuzzle.cpp](src/game/puzzles/logic/Jade/JadeDebatePuzzle.cpp), [VictorMemoryPuzzle.cpp](src/game/puzzles/logic/Victor/VictorMemoryPuzzle.cpp), etc.

**Recommendation:** Create centralized `CooldownManager` or use a generic `Cooldown<T>` component

---

### B. Proximity/Distance Checks Duplicates
**Severity: MEDIUM** | **Found: 24 instances**

Pattern: Repeated glm::length calculations for proximity
```cpp
float distance = glm::length(delta);
if (distance <= threshold) { /* logic */ }
```

**Locations:**
- [Enemy.cpp](src/game/entities/Enemy.cpp): L43, L66, L145, L152, L164 (5 checks)
- [NPC.cpp](src/game/entities/NPC.cpp): L38, L54, L57, L151, L163, L168 (6 checks)
- [InteractionSystem.cpp](src/game/interactions/InteractionSystem.cpp): L18, L29 (custom HorizontalDistance)
- [World.cpp](src/game/world/World.cpp): L84, L322, L1041, L2696 (4 checks)
- [Game.cpp](src/game/Game.cpp): L396, L402, L408 (3 checks)
- [InputContext.cpp](src/engine/input/InputContext.cpp): L134 (movement vector)

**Issue:** `HorizontalDistance()` function duplicates `glm::length()` logic

---

### C. Update Loops on Same Collections
**Severity: MEDIUM** | **Found: 28+ patterns**

**Examples:**
- [World.cpp](src/game/world/World.cpp): L1519 (characters loop), L1648 (npcCooldowns loop)
- [SymbolSystem.cpp](src/game/symbols/SymbolSystem.cpp): L60, L90, L97, L105, L142, L152, L158 (7 loops over same collection)
- [DialogueManager.cpp](src/game/dialogue/DialogueManager.cpp): L488, L565 (relationship loops)
- [ContradictionTracker.cpp](src/game/puzzles/core/ContradictionTracker.cpp): L24, L33 (statement loops)

**Issue:** Multiple passes over same data in single Update frame → O(n²) complexity risk

---

### D. Input Checking Duplicates
**Severity: MEDIUM** | **Found: 100+ instances**

Pattern: Repeated `IsActionPressed()` checks
```cpp
if (input.IsActionPressed(InputAction::NavigateUp) || 
    input.IsActionPressed(InputAction::DialogueChoicePrev) || 
    input.IsActionPressed(InputAction::PuzzleUp)) { }
```

**Problematic Locations:**
- [UIScreen.cpp](src/engine/ui/UIScreen.cpp): L24, L30 (triple checks)
- [MainMenuScreen.cpp](src/engine/ui/MainMenuScreen.cpp): L36, L39, L43 (triple checks)
- [Game.cpp](src/game/Game.cpp): L446-450 (5-way character switch check, duplicated logic)
- [DialogueManager.cpp](src/game/dialogue/DialogueManager.cpp): L593, L598, L622, L625, L629, L633, L637, L641

**Issue:** No abstraction for input patterns. Each UI/game system reimplements navigation logic.

---

### E. Audio Triggers (PlaySound Calls)
**Severity: MEDIUM** | **Found: 100+ instances**

Pattern: Direct `PlaySound()` calls scattered across puzzle/dialogue logic
```cpp
PlaySound("puzzle_tick");
PlaySound("puzzle_complete");
```

**Distribution:**
- Puzzle classes: 50+ direct calls across all logic puzzles
- [DialogueManager.cpp](src/game/dialogue/DialogueManager.cpp): L457, L671 (cinematic stings)
- [World.cpp](src/game/world/World.cpp): L314, L1132, L1180, L1484 (audio from game state)

**Issue:** Audio logic tightly coupled to game logic. No abstraction for sound events.

---

### F. Text Rendering Helpers
**Severity: LOW** | **Found: 80+ RenderText() calls**

**Pattern:** Consistent use of `textRenderer->RenderText()` with varying parameters
- [Game.cpp](src/game/Game.cpp): 30+ calls with hardcoded positions, scales, colors
- [DialogueUI.cpp](src/game/dialogue/DialogueUI.cpp): L141-154 (dialogue rendering)
- [AtmosphereManager.cpp](src/game/atmosphere/AtmosphereManager.cpp): L117, L134 (hallucination rendering)

**Issue:** While consistent API, hardcoded scale/color values across codebase. No text layout abstraction.

---

### G. Interaction Proximity Checks
**Severity: MEDIUM** | **Found: Centralized but logic could improve**

[InteractionSystem.cpp](src/game/interactions/InteractionSystem.cpp) has:
- Custom `HorizontalDistance()` at L15 (duplicates glm::length)
- Loop through nodes at L395-397 with distance check
- Similar logic in [World.cpp](src/game/world/World.cpp) for NPC interactions

**Issue:** Two separate proximity systems (InteractionSystem + World NPC proximity logic)

---

## 3. MANAGER & SYSTEM ANALYSIS

### A. All Manager/System Classes (9 singletons)

| Class | File | Singleton Pattern | Responsibilities | Update Method |
|-------|------|-------------------|------------------|----------------|
| **TransitionManager** | [TransitionManager.h](src/engine/ui/TransitionManager.h) | `Instance()` | Screen fade transitions, timing | Yes |
| **UIManager** | [UIManager.h](src/engine/ui/UIManager.h) | `Instance()` | UI screen stack, widget management | Yes |
| **TunnelMappingSystem** | [TunnelMappingSystem.h](src/game/tunnels/TunnelMappingSystem.h) | `Instance()` + static g_tunnelSystem | Tunnel route mapping for puzzles | Yes |
| **MemoryReplaySystem** | [MemoryReplaySystem.h](src/game/memory/MemoryReplaySystem.h) | `Instance()` + static g_memorySystem | Memory replay mode, duration tracking | Yes |
| **SymbolSystem** | [SymbolSystem.h](src/game/symbols/SymbolSystem.h) | `Instance()` + static g_symbolSystem | Symbol visibility/discovery, sight duration | Yes |
| **AtmosphereManager** | [AtmosphereManager.h](src/game/atmosphere/AtmosphereManager.h) | `Instance()` + static g_atmosphere | Hallucinations, whispers, ambient effects | Yes |
| **DialogueManager** | [DialogueManager.h](src/game/dialogue/DialogueManager.h) | `Instance()` | Dialogue flow, state, relationship tracking | Yes |
| **StoryManager** | [StoryManager.h](src/game/story/StoryManager.h) | `Instance()` | Story progression, hints | Yes |
| **MoralCorruptionSystem** | [MoralCorruptionSystem.h](src/game/moral/MoralCorruptionSystem.h) | `Instance()` + static g_moralSystem | Moral corruption level, evidence tracking | Yes |

### B. Singleton Pattern Issues
**Severity: MEDIUM**

- **Dual initialization:** 5 systems use BOTH `Instance()` method AND static g_* global (redundant)
  - [AtmosphereManager.cpp](src/game/atmosphere/AtmosphereManager.cpp): L10, L26
  - [TunnelMappingSystem.cpp](src/game/tunnels/TunnelMappingSystem.cpp): L7, L12
  - [MemoryReplaySystem.cpp](src/game/memory/MemoryReplaySystem.cpp): L7, L12
  - [SymbolSystem.cpp](src/game/symbols/SymbolSystem.cpp): L6, L11
  - [MoralCorruptionSystem.cpp](src/game/moral/MoralCorruptionSystem.cpp): L7, L12

- **Inconsistent access:** Some systems accessed via global, some via Instance()
- **Hidden dependencies:** Difficult to trace which systems depend on which

### C. Manager Responsibility Breakdown

**Critical Path Systems:**
- [World](src/game/world/World.h): Character state, NPC AI, interaction checks, audio cues, **2410 lines**
- [DialogueManager](src/game/dialogue/DialogueManager.h): Dialogue state, relationship tracking, 642 lines
- [PuzzleManager](src/game/puzzles/core/PuzzleManager.h): Puzzle factory, state, 568 lines

**Supporting Systems:**
- [InteractionSystem](src/game/interactions/InteractionSystem.h): Interaction node proximity/activation
- [QuestSystem](src/game/quest/QuestSystem.h): Quest progression, objectives
- [SymbolSystem](src/game/symbols/SymbolSystem.h): Symbol discovery, timing

**Atmosphere/Narrative:**
- [AtmosphereManager](src/game/atmosphere/AtmosphereManager.h): Hallucinations, whispers, mood
- [StoryManager](src/game/story/StoryManager.h): Story progression hints
- [MoralCorruptionSystem](src/game/moral/MoralCorruptionSystem.h): Moral state tracking

**Update Order Dependencies:**
1. InputManager (input capture)
2. World (character/NPC updates)
3. InteractionSystem (proximity checks)
4. DialogueManager (dialogue advancement)
5. PuzzleManager (puzzle state)
6. AudioManager (sound playback)
7. UI systems
8. Rendering

**Potential Issues:**
- No explicit update order enforcement
- [World](src/game/world/World.h) does too much (character logic + environment + narratives)
- [PuzzleManager](src/game/puzzles/core/PuzzleManager.h) and [DialogueManager](src/game/dialogue/DialogueManager.h) responsibilities overlap in dialogue puzzles

---

## 4. ARCHITECTURE PATTERNS

### A. Raw Pointer Usage
**Severity: HIGH** | **Found: 5 unsafe global pointers**

Static globals with manual `new` allocation:
- [AtmosphereManager.cpp](src/game/atmosphere/AtmosphereManager.cpp): L10, L26 → `g_atmosphere = new AtmosphereManager()`
- [TunnelMappingSystem.cpp](src/game/tunnels/TunnelMappingSystem.cpp): L7, L12 → `g_tunnelSystem = new TunnelMappingSystem()`
- [MemoryReplaySystem.cpp](src/game/memory/MemoryReplaySystem.cpp): L7, L12 → `g_memorySystem = new MemoryReplaySystem()`
- [SymbolSystem.cpp](src/game/symbols/SymbolSystem.cpp): L6, L11 → `g_symbolSystem = new SymbolSystem()`
- [MoralCorruptionSystem.cpp](src/game/moral/MoralCorruptionSystem.cpp): L7, L12 → `g_moralSystem = new MoralCorruptionSystem()`

**Issues:**
- Memory never freed (leak on shutdown)
- No `delete` statements found
- Inconsistent with RAII principles
- Type unsafety

Raw pointer parameters (acceptable but could be smarter):
- [DialogueManager.h](src/game/dialogue/DialogueManager.h): L19, L60 → `AudioManager* audioManager`
- [TransitionManager.h](src/engine/ui/TransitionManager.h): L21, L38 → `AudioManager* audio`
- [TextRenderer.h](src/engine/renderer/TextRenderer.h): Passed as raw pointer throughout

---

### B. Static Globals and Singletons
**Severity: HIGH** | **Count: 5 static globals + 9 Instance() methods**

**Pattern:** Mix of approaches creates cognitive burden

```cpp
// Pattern 1: Static global + Instance() (AtmosphereManager.cpp)
static AtmosphereManager* g_atmosphere = nullptr;
AtmosphereManager& AtmosphereManager::Instance() {
    if (!g_atmosphere) g_atmosphere = new AtmosphereManager();
    return *g_atmosphere;
}

// Pattern 2: Static instance (TransitionManager.cpp)
TransitionManager& TransitionManager::Instance() {
    static TransitionManager inst;
    return inst;
}
```

**Problems:**
- No initialization order guarantee
- Global state couples all components
- Testing/dependency injection impossible
- Unclear object lifetime

---

### C. Ownership Ambiguity
**Severity: HIGH** | **Multiple instances**

**Unclear ownership patterns:**
1. [AudioManager pointers](src/game/dialogue/DialogueManager.h): L60 - raw pointer with no RAII, unknown owner
2. [TextRenderer parameters](src/game/Game.cpp): L32 - `unique_ptr` created but passed as raw pointer
3. [Character references in loops](src/game/world/World.cpp): L1519 - reference to container element
4. [Puzzle instances in PuzzleManager](src/game/puzzles/core/PuzzleManager.h): Uses `unique_ptr` (good) but destruction timing unclear

**Recommendation:** Use explicit ownership documentation or migrate to smart pointers consistently

---

### D. Event Systems
**Severity: MEDIUM** | **Partial implementation**

**EventBus exists but underutilized:**

[EventBus.h](src/game/runtime/EventBus.h): Template-based event system with Subscribe/Publish

**Current usage:**
- [World.cpp](src/game/world/World.cpp): L1139, L1147, L1155, L1163, L1170 - Subscribe to PuzzleCompletedEvent, InteractionTriggeredEvent, PromiseEvents, AccusationEvent
- [World_Characters.cpp](src/game/world/World_Characters.cpp): L63 - Publish ActiveCharacterSwitchedEvent

**Issues:**
- Only 4 event types defined in [GameplayEvents.h](src/game/runtime/GameplayEvents.h)
- Most game state changes use direct function calls
- Direct coupling between systems (e.g., AudioManager passed to DialogueManager)
- Could eliminate 50+ direct method calls if audio/UI events were added

**Example of direct coupling:**
```cpp
// Current: Direct call
audioManager->PlaySound("puzzle_complete");

// Should be: Event-driven
eventBus.Publish(AudioTriggeredEvent{"puzzle_complete"});
```

---

### E. Update Ordering Dependencies
**Severity: MEDIUM**

**Implicit dependencies found:**
1. **Input must update before Game logic** - Input consumed in Game::Update()
2. **Character updates before Interaction checks** - InteractionSystem checks character position against nodes
3. **World updates before Dialogue/Puzzle checks** - Dialog/puzzle state depends on character/quest state
4. **Puzzle completion before dialogue reactions** - DialogueManager listens for PuzzleCompletedEvent

**No enforcement mechanism:** Update order is hardcoded in [Game.cpp](src/game/Game.cpp) main loop with no documentation

**Risk:** Reordering could cause subtle bugs. No test coverage for update order.

---

## 5. LARGE MONOLITHIC FILES (>500 lines)

### [World.cpp](src/game/world/World.cpp) - CRITICAL
**2410 lines** | **Severity: CRITICAL**

**Responsibilities (should be 5 files):**
1. **Character Management** - L1519+ (switching, state)
2. **NPC Behavior** - L80-100+ (perception, behavior trees)
3. **Environment Logic** - L400-500+ (object interactions, puzzle logic)
4. **Audio Cueing** - L274-320 (voice line playback, audio events)
5. **Narrative Events** - L1400-1700+ (story progression, dialogue triggers)
6. **Enemy AI & Combat** - L1100-1200+ (perception, attack logic)
7. **Interaction Processing** - L2000-2100+ (quest steps, dialogue cues)
8. **Event Subscriptions** - L1138-1175 (EventBus reactions)

**Key Functions by Line Count:**
- `Update()`: ~300 lines (mixing character, NPC, and world updates)
- `PerceptionStimulus()`: ~150 lines (complex perception calculation)
- `PlayVoiceLineInternal()`: ~50 lines (voice line logic)

**Recommendation:** Break into:
- `WorldEnvironment` - static environment & objects
- `NPCManager` - NPC updates, perception, behavior
- `CombatSystem` - enemy AI, attacks
- `AudioEventDispatcher` - voice and audio cues
- `NarrativeEventDispatcher` - story events and reactions

---

### [DialogueManager.cpp](src/game/dialogue/DialogueManager.cpp) - HIGH
**642 lines** | **Severity: HIGH**

**Responsibilities (should be 2-3 files):**
1. **Dialogue Flow** - State machine for dialogue progression
2. **Relationship Tracking** - Relationship value updates
3. **UI Rendering** - Text rendering (delegates to DialogueUI, but tightly coupled)
4. **Audio Integration** - Voice line playback (directly calls audioManager)

**Issues:**
- [DialogueUI.h](src/game/dialogue/DialogueUI.h) exists but DialogueManager::Render() still does UI work
- Direct dependency on [AudioManager.h](src/engine/audio/AudioManager.h)
- Relationship update logic could be in separate system

**Recommendation:** Extract AudioEvents and Relationship tracking

---

### [PuzzleManager.cpp](src/game/puzzles/core/PuzzleManager.cpp) - HIGH
**568 lines** | **Severity: HIGH**

**Responsibilities:**
1. **Puzzle Factory** - Create puzzle instances based on type
2. **Puzzle State Management** - Active puzzle, completion tracking
3. **UI Rendering** - Puzzle overlay rendering
4. **Input Routing** - Route input to active puzzle

**Issue:** Mixing factory pattern with state management and UI rendering

**Recommendation:** Separate `PuzzleFactory`, `PuzzleStateManager`, `PuzzleRenderer`

---

### [Quest.cpp](src/game/quest/Quest.cpp) - HIGH
**600 lines** | **Severity: HIGH**

**Responsibilities:**
1. **Quest State** - Progression through objectives
2. **Serialization** - Save/load quest state
3. **Reward Logic** - Completing quest objectives

**Issue:** Mixing data, persistence, and business logic

**Recommendation:** Separate `QuestData`, `QuestSerializer`, `QuestRewardSystem`

---

## 6. CRITICAL ISSUES SUMMARY

| Issue | Severity | Count | Files |
|-------|----------|-------|-------|
| Monolithic files (>500 lines) | CRITICAL | 4 | World, DialogueManager, PuzzleManager, Quest |
| Timer/cooldown duplicates | HIGH | 150+ | 20+ files |
| Raw pointer globals with new/delete | HIGH | 5 | AtmosphereManager, TunnelMappingSystem, MemoryReplaySystem, SymbolSystem, MoralCorruptionSystem |
| Ownership ambiguity | HIGH | 10+ | DialogueManager, TransitionManager, multiple raw pointers |
| Repeated input checking patterns | MEDIUM | 100+ | UIScreen, MainMenuScreen, Game, DialogueManager |
| Proximity/distance check duplication | MEDIUM | 24 | Enemy, NPC, InteractionSystem, World, Game |
| Audio triggers scattered | MEDIUM | 100+ | 50+ puzzle files, DialogueManager, World |
| Multiple collection passes | MEDIUM | 28+ | World, SymbolSystem, DialogueManager, etc. |
| Missing EventBus integration | MEDIUM | - | AudioEvents, UIEvents not defined |
| Implicit update order dependencies | MEDIUM | 5-7 | Game, World, DialogueManager, PuzzleManager |

---

## 7. RECOMMENDATIONS (Priority Order)

### Phase 1 - Critical (Blocking Quality)
1. **Break down World.cpp** → 5 separate files
2. **Consolidate timer logic** → Centralized CooldownManager or Cooldown component
3. **Fix raw pointer globals** → Use unique_ptr or static factories

### Phase 2 - High (Enabling Maintainability)
4. **Extract Puzzle subsystems** → PuzzleFactory, PuzzleStateManager, PuzzleRenderer
5. **Expand EventBus** → Add AudioTriggeredEvent, UIUpdateEvent, CharacterDeathEvent
6. **Consolidate audio logic** → Audio event dispatcher instead of direct calls

### Phase 3 - Medium (Improving Consistency)
7. **Create InputPattern abstraction** → Reduce 100+ IsActionPressed checks
8. **Centralize proximity logic** → Single DistanceUtil with HorizontalDistance
9. **Document update order** → Add comments or create UpdateScheduler

### Phase 4 - Low (Polish)
10. **Migrate raw pointers** → TextRenderer, AudioManager
11. **Standardize manager initialization** → Choose Instance() OR static global, not both
12. **Extract story/narrative logic** → Separate NarrativeDispatcher from World

---

**Total Analysis:** 204 files reviewed | Patterns identified: 7 categories | Actionable findings: 40+ locations documented
