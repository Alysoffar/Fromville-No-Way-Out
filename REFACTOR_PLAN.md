# 🏗️ PROFESSIONAL CODEBASE REFACTOR PLAN
## Transform Prototype → Production-Quality Code

**Status:** Planning Phase
**Timeline:** 6 Phases (incremental, builds maintained throughout)
**Goal:** Clean, scalable, maintainable AAA-quality horror game architecture

---

## PHASE 0: FOUNDATION (Stability & Tools)
### Goals: Create safe refactor infrastructure
- [ ] Create utility namespace for shared helpers (timers, math, collections)
- [ ] Add debug stats tracking (FPS, entity count, interaction count)
- [ ] Create build validation script
- [ ] Add compiler warning fixes (-Wunused-parameter, etc)
- [ ] Document current update pipeline
**Estimate:** 2-3 hours | **Risk:** LOW | **Compile:** After each step

---

## PHASE 1: UNSAFE GLOBALS FIX (Memory Safety)
### Goals: Eliminate memory leaks, fix raw pointers
**Critical Issue:** 5 raw pointer globals with no delete
- g_atmosphere, g_tunnelSystem, g_memorySystem, g_symbolSystem, g_moralSystem

**Approach:**
1. Create safe factory functions in World
2. Use unique_ptr ownership
3. Store in World, not globals
4. Update all references (10-20 locations)
5. Remove manual new/delete
**Estimate:** 4-5 hours | **Risk:** MEDIUM | **Compile:** 3-4 checkpoints

---

## PHASE 2: COOLDOWN/TIMER CENTRALIZATION (Duplication)
### Goals: Remove 150+ duplicate timer loops
**Current Issue:** Each system has manual `cooldown -= dt` loops scattered everywhere

**Solution:**
1. Create reusable `Cooldown` and `DurationTimer` class
2. Audit all timer usage in:
   - World.cpp (characterDamageCooldowns, npcTalkCooldowns, etc)
   - Characters (attack, crouch, sprint timers)
   - NPCs (talk cooldowns, patrol timers)
   - Enemies (aggro timers, patrol timers)
   - Puzzles (phase timers, UI timers)
3. Replace manual loops with component-based updates
4. Centralize cooldown reset logic
**Estimate:** 6-8 hours | **Risk:** MEDIUM | **Compile:** 5-6 checkpoints

---

## PHASE 3: WORLD.CPP BREAKDOWN (Monolithic Code)
### Goals: Split 2410-line World.cpp into focused subsystems
**Current Problems:** 
- World handles: characters, NPCs, audio, narratives, interactions, enemies, puzzles, quest sync
- 40+ member variables mixing concerns
- Update() is 200+ lines with implicit ordering

**Structure After:**
```
World.h/cpp                    (orchestration, subsystem access)
  ├── WorldCharacterSystem     (character switching, movement, targeting)
  ├── WorldNPCSystem           (NPC spawning, updates, interactions)
  ├── WorldAudioSystem         (ambient music, voice cues, transitions)
  ├── WorldNarrativeSystem     (dialogue integration, story flags)
  ├── WorldInteractionSystem   (proximity checks, object targeting)
  └── WorldEnvironmentSystem   (terrain, zones, shelter checks)
```

**Approach:**
1. Extract character subsystem (200 lines) → WorldCharacterSystem
2. Extract NPC subsystem (250 lines) → WorldNPCSystem
3. Extract narrative subsystem (300 lines) → WorldNarrativeSystem
4. Extract interaction logic (150 lines) → consolidated in existing InteractionSystem
5. Clean up World to delegate, not implement
6. Create clear update() pipeline with phases

**Estimate:** 10-12 hours | **Risk:** HIGH (many interdependencies) | **Compile:** 8-10 checkpoints

---

## PHASE 4: PUZZLE SYSTEM CONSOLIDATION (Architecture)
### Goals: Reduce 568-line PuzzleManager, remove 100+ audio trigger duplicates
**Current Issues:**
- PuzzleManager mixes state, logic, and audio triggers
- Each puzzle re-implements solve checks, audio cues
- Audio triggers scattered (40+ `PlaySound()` calls in puzzle logic)

**Approach:**
1. Create PuzzleAudioManager for centralized cues
2. Create BasePuzzle standardized audio hooks (onEnter, onProgress, onSolve)
3. Extract puzzle state from PuzzleManager → PuzzleStateManager
4. Remove direct audio calls from puzzle logic
5. Route audio through event system
**Estimate:** 6-7 hours | **Risk:** MEDIUM | **Compile:** 5-6 checkpoints

---

## PHASE 5: INPUT/EVENT SYSTEM CONSOLIDATION (Coupling)
### Goals: Remove duplicated input checks, expand EventBus
**Current Issues:**
- 100+ `IsActionPressed()` scattered
- Navigation logic duplicated in every UI/puzzle
- Audio events not in EventBus (direct calls instead)

**Approach:**
1. Create InputRouter (centralizes input branching)
2. Create UIInputContext (semantic nav handling)
3. Add missing events to EventBus:
   - AudioTriggeredEvent
   - InteractionStartedEvent
   - PuzzlePhaseChangedEvent
   - DialogueLinePlayedEvent
4. Replace 50+ direct calls with event dispatch
5. Remove redundant navigation checks (consolidate into 1 place)
**Estimate:** 5-6 hours | **Risk:** MEDIUM | **Compile:** 4-5 checkpoints

---

## PHASE 6: AUDIO & UI OWNERSHIP CLEANUP (Architecture)
### Goals: Fix AudioManager/TextRenderer ownership, event-drive everything
**Current Issues:**
- AudioManager injected as raw pointer everywhere
- TextRenderer passed into 20+ places
- Audio fade logic split between TransitionManager and AudioManager

**Approach:**
1. Create AudioService singleton (safe wrapper)
2. Create UIService singleton (safe renderer access)
3. Audio fades → pure AudioService responsibility
4. UI rendering → route through UIManager only
5. Remove raw pointer passing from gameplay code
6. Create audio cue registry (standardized naming)
**Estimate:** 4-5 hours | **Risk:** MEDIUM | **Compile:** 3-4 checkpoints

---

## PHASE 7: INTERACTION SYSTEM POLISH (Architecture)
### Goals: Consolidate 24 proximity checks, remove duplication
**Current Issues:**
- Proximity checks scattered in World.cpp, Character.cpp, Puzzle.cpp
- Interaction prioritization unclear
- Repeated `glm::distance()` calculations

**Approach:**
1. Create InteractionQuery utility (cached proximity checks)
2. Centralize interaction prioritization (distance + type)
3. Cache results per frame (dirty update)
4. Remove duplicated checks
5. Create consistent interaction event dispatch
**Estimate:** 3-4 hours | **Risk:** LOW-MEDIUM | **Compile:** 3 checkpoints

---

## PHASE 8: CODE STYLE & NAMING CONSISTENCY (Polish)
### Goals: Make codebase feel cohesive and professional
**Focus Areas:**
- Standardize naming: gVariable, m_member, camelCase
- Method ordering: public → protected → private
- Class organization: data → initialization → updates → rendering
- Consistent formatting and brace styles
- Remove unused parameters (compiler warnings)

**Approach:**
1. Create .clang-format file for standardized formatting
2. Audit and rename inconsistent names (5-10 files)
3. Organize class methods consistently
4. Fix remaining compiler warnings
**Estimate:** 3-4 hours | **Risk:** LOW | **Compile:** 2-3 checkpoints

---

## PHASE 9: DOCUMENTATION & DEBUG FOUNDATION (Quality)
### Goals: Add professional documentation, debug overlay
**Approach:**
1. Add subsystem ownership documentation (comments)
2. Create debug overlay system:
   - FPS counter
   - Entity count
   - Active interactions
   - Event counts
   - Timing measurements
3. Document update pipeline in World
4. Add architecture notes to key files
**Estimate:** 4-5 hours | **Risk:** LOW | **Compile:** 2-3 checkpoints

---

## PHASE 10: PERFORMANCE OPTIMIZATION PASS (Performance)
### Goals: Eliminate inefficiencies (not premature optimization)
**Focus:**
- Cache expensive queries (NPC lookups)
- Reduce repeated proximity scans (batch with distance checks)
- Lazy load puzzle state
- Reduce audio queries per frame
- Profile update phases

**Approach:**
1. Add timing measurements to update phases
2. Identify hot spots
3. Implement dirty updates where applicable
4. Cache static queries
**Estimate:** 4-5 hours | **Risk:** LOW | **Compile:** 3-4 checkpoints

---

## PHASE 11: VALIDATION & POLISH (Testing)
### Goals: Ensure refactored code is stable and gameplay unchanged
**Approach:**
1. Full playthrough test (all 4 characters, all quests)
2. Verify all puzzles functional
3. Verify all interactions responsive
4. Verify audio fades/stingers working
5. Memory leak detection
6. Performance comparison (before/after)
7. Final build verification

**Estimate:** 3-4 hours | **Risk:** LOW | **Compile:** Final validation

---

## IMPLEMENTATION STRATEGY

### Incremental Approach
- Each phase: small tasks → compile → test → commit
- Maintain playable builds at all times
- No "massive rewrites" - systematic refactoring
- Frequent validation

### Compile Gates
- After every 2-3 file changes
- Run full Release build
- Verify no new errors/warnings
- Quick gameplay smoke test

### Risk Management
- **PHASE 0-1:** Safest (foundation, memory fixes)
- **PHASE 2-3:** Medium risk (major refactoring, many interdependencies)
- **PHASE 4-7:** Moderate risk (system consolidation, but isolated)
- **PHASE 8-11:** Low risk (polish, optimization, testing)

### Rollback Plan
- Git commits after each compile gate
- If phase breaks builds, revert to last stable point
- Never push broken code

---

## SUCCESS METRICS

**Pre-Refactor:**
- 2410-line World.cpp
- 150+ duplicate timers
- 100+ duplicate input checks
- 50+ direct audio calls
- 5 unsafe globals
- Implicit update ordering
- Mixed singleton patterns

**Post-Refactor:**
- World.cpp < 600 lines (delegates to subsystems)
- Centralized cooldown manager
- Consolidated input routing
- Event-driven audio
- Zero unsafe globals
- Explicit update phases
- Consistent architecture

---

## TOTAL ESTIMATE
**~60-70 hours of focused refactoring work**

Prioritized by:
1. **Stability impact** (unsafe globals)
2. **Code duplication removal** (timers, input)
3. **Architecture clarity** (World breakdown)
4. **Maintenance improvement** (consolidation)
5. **Professional polish** (style, docs)

---

## DO NOT
- Change gameplay behavior
- Over-engineer solutions
- Add unnecessary abstraction layers
- Break builds
- Commit without compiling

---

## PHASE CHECKLIST

- [ ] Phase 0: Foundation (2-3 hrs)
- [ ] Phase 1: Unsafe Globals (4-5 hrs)
- [ ] Phase 2: Cooldown Centralization (6-8 hrs)
- [ ] Phase 3: World.cpp Breakdown (10-12 hrs)
- [ ] Phase 4: Puzzle Consolidation (6-7 hrs)
- [ ] Phase 5: Input/Event System (5-6 hrs)
- [ ] Phase 6: Audio/UI Ownership (4-5 hrs)
- [ ] Phase 7: Interaction System (3-4 hrs)
- [ ] Phase 8: Code Style (3-4 hrs)
- [ ] Phase 9: Documentation (4-5 hrs)
- [ ] Phase 10: Performance (4-5 hrs)
- [ ] Phase 11: Validation (3-4 hrs)

**Ready to begin Phase 0 when you approve.**
