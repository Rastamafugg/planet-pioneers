# Implementation Roadmap

Phase plan and timeline for building Planet Pioneers from the existing PoCs to a complete game. Captures the architectural choices made on 2026-04-24 after ingesting the [stocks-and-bonds IPC reference](../sources/stocks-and-bonds.md).

## Architectural baseline

- **Multi-process target:** logic process + render child + sound child, communicating via signals + shared state. See [platform/ipc.md](../platform/ipc.md).
- **Fallback:** 2-process (logic+sound) if cross-process shared memory for render proves infeasible. Decided by `poc_shmem`.
- **Initial scope simplification:** single human player + 3 AI opponents, keyboard-only. Joystick and multi-keyboard / additional humans are explicit polish work, not core.
- **Sound:** dedicated child process driven by signals; payload via shared state. Pattern ported from `SLPICPT.c`.

## Critical-path risks (front-loaded)

1. **Cross-process shared memory for render** — mechanism (`F$AllRAM` + `F$MapBlk` with bearer block-IDs) confirmed against kernel source on 2026-04-24. `poc_shmem` (phase 2b) now validates the C-side implementation, not the kernel feasibility. 3P architecture is the working baseline; 2P fallback only if `poc_shmem` reveals an unexpected DCC/libc obstacle.
2. **64 KB process budget** — measure code size after every milestone, not at the end.
3. **Commodity auction phase** — 4 simultaneous price lines + input is the hardest interactive code in the game; gets its own PoC inside phase 7.
4. **AI must land alongside phases**, not after — single human means 3 AI opponents are required from the first playable phase.

## Phases

| #   | Phase                                   | Deliverable                                                                | Effort  | Notes |
|-----|------------------------------------------|----------------------------------------------------------------------------|---------|-------|
| 1   | Core skeleton ✅                          | `main.c`, [`GameState`](data-structures.md), turn-phase state machine. Done 2026-04-25 — `pioneer` runs cleanly; `direct` placement deferred to its own PoC | 0.5 wk  | |
| 2a  | **`poc_ipc`** ⚠ gating                  | Port `SLPICPT.c`; fork child; round-trip a signal in C. Built; live-test deferred | 0.5 wk  | Direct port from stocks-and-bonds |
| 2b  | **`poc_shmem`**                          | Two processes share a memory region via `F$AllRAM` + `F$MapBlk` (block-number passing); see [platform/ipc.md](../platform/ipc.md) | 1 wk    | Mechanism confirmed in Tech Ref + kernel source 2026-04-24; PoC validates the pattern in C |
| 3   | Sound child process                       | `sound/` module; `{freq,dur,amp}` queue; non-blocking from logic POV       | 0.5 wk  | De-risked by 2a |
| 4   | Render module / process                   | Promote `poc_cvdg16` → `render/` (page flip + tile + sprite + palette)     | 1–1.5 wk | Path depends on 2b outcome |
| 5   | Input — single keyboard                   | `SS.KySns` polling, key-event dispatch                                     | 0.5 wk  | |
| 6   | Minimal AI                                | Stub policies for management, auction, land grant (good enough to play against) | 1.5 wk  | Refined per-phase in 7 |
| 7a  | Phase: Summary / Scoring                  | Net-worth calc + render — integration smoke test                           | 0.5 wk  | Easiest phase first |
| 7b  | Phase: Land Grant                         | Cursor, selection, simultaneous-press tie-break                            | 0.5 wk  | |
| 7c  | Phase: Colony Management                  | The MULE walk — heaviest reuse of `poc_cvdg16`                             | 1.5 wk  | |
| 7d  | Phase: Production                         | Pure logic — base + EOS/LCT bonuses + spoilage                             | 1 wk    | Needs GDD §12 ingest |
| 7e  | Phase: Land Auction (Std/Tournament)      |                                                                            | 0.5 wk  | |
| 7f  | **Phase: Commodity Auctions** ⚠         | 4-player real-time price lines — own PoC first                             | 2 wk    | Hardest phase. Needs GDD §14 ingest |
| 7g  | Phase: Random Events                      | Table-driven event dispatch                                                | 0.5 wk  | Needs GDD §13 ingest |
| 8   | Polish: joystick, 2nd keyboard, more humans | Generalize input dispatch; UI for player select                          | 1 wk    | |
| 9   | Tournament rules + balance + memory audit  | Collusion, crystite, mode gates; `buildc` map review                       | 1 wk    | |
| 10  | QA pass                                   | Golden-path playtests, regression vs. manual & strategy-guide behavior     | 1 wk    | Mandatory closing role |

**Critical path (1 → 2a → 2b → 3 → 4 → 5 → 6 → 7a..g → 8 → 9 → 10): ~13–14 effort-weeks.**

## Dependency graph

```
1 ──► 2a ──► 2b ──► 4 ──┐
              │         │
              └──► 3 ───┤
                        │
                  5 ────┤
                        │
                  6 ────┴──► 7a → 7b → 7c → 7d → 7e → 7f → 7g → 8 → 9 → 10
```

`poc_shmem` (2b) gates render path. Sound (3) and render (4) can run in parallel after 2b lands. AI (6) must precede first playable phase (7a).

## Ingest dependencies

Schedule **before** the phase that needs them, not up-front:

| Before phase | Must ingest                                                  |
|--------------|--------------------------------------------------------------|
| 2a / 2b      | NitrOS-9 Tech Ref §F$Fork, F$Send, F$Sleep, F$Wait, F$AllRam, F$MapBlk |
| 7d           | GDD §12 (Production) — currently stub                        |
| 7e / 7f      | GDD §14 (Commodity Auctions) — currently stub                |
| 7g           | GDD §13 (Random Events) — currently stub                     |
| 9            | GDD §15–18 (Economy, Tournament, AI, Scoring)                |

## Done-criteria per phase

Every phase finishes with a `qa-reviewer` pass per [agent-workflow.md](agent-workflow.md). Implementation phases must also:
- Update [poc-catalog.md](poc-catalog.md) when a new PoC is added.
- Append observed-fact findings to [lessons-learned.md](lessons-learned.md).
- Verify the program still fits the [memory budget](../platform/memory.md) (`buildc` map output recorded in the phase's commit message).

## Sources

- [poc-catalog.md](poc-catalog.md), [data-structures.md](data-structures.md), [lessons-learned.md](lessons-learned.md)
- [platform/ipc.md](../platform/ipc.md), [platform/memory.md](../platform/memory.md)
- [sources/stocks-and-bonds.md](../sources/stocks-and-bonds.md)
- `docs/design/MULE_GDD.md` §22 (function signatures), §23 (memory)
