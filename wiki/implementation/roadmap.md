# Implementation Roadmap

Phase plan and timeline for building Planet Pioneers from the existing PoCs to a complete game. Captures the architectural choices made on 2026-04-24 after ingesting the [stocks-and-bonds IPC reference](../sources/stocks-and-bonds.md).

## Architectural baseline

- **Multi-process target:** logic process + render child + sound child, communicating via signals + shared state. See [platform/ipc.md](../platform/ipc.md).
- **Fallback:** 2-process (logic+sound) if cross-process shared memory for render proves infeasible. Decided by `poc_shmem`.
- **Initial scope simplification:** single human player + 3 AI opponents, keyboard-only. Joystick and multi-keyboard / additional humans are explicit polish work, not core.
- **Sound:** dedicated child process driven by signals; payload via shared state. Pattern ported from `SLPICPT.c`.

## Critical-path risks (front-loaded)

1. ~~**Cross-process shared memory for render**~~ ✅ resolved 2026-04-25 — `poc_shmem` C-side passed live-test on EOU. **3-process architecture (logic + render + sound) is the locked-in baseline.** 2P fallback no longer in play.
2. **64 KB process budget** — measure code size after every milestone, not at the end.
3. **Commodity auction phase** — 4 simultaneous price lines + input is the hardest interactive code in the game; gets its own PoC inside phase 7.
4. **AI lands inside each 7x phase** — 3 CPU opponents are required from the first playable phase that needs decisions, but the original phase-6 "AI module up front" plan was dissolved 2026-04-26 (see [ai.md](../game/ai.md) decision note). Each AI policy now ships with its consumer.

## Phases

| #   | Phase                                   | Deliverable                                                                | Effort  | Notes |
|-----|------------------------------------------|----------------------------------------------------------------------------|---------|-------|
| 1   | Core skeleton ✅                          | `main.c`, [`GameState`](data-structures.md), turn-phase state machine. Done 2026-04-25 — `pioneer` runs cleanly; `direct` placement deferred to its own PoC | 0.5 wk  | |
| 2a  | **`poc_ipc`** ✅                          | Port `SLPICPT.c`; fork child; round-trip a signal in C. Live-test passed 2026-04-25 — F$Fork + F$Send + F$Wait round-trip confirmed | 0.5 wk  | Direct port from stocks-and-bonds; resolved [ipc.md](../platform/ipc.md) open question #3 |
| 2b  | **`poc_shmem`** ✅                        | Two processes share a memory region via `F$AllRAM` + `F$MapBlk`. Live-test passed 2026-04-25 — full lifecycle (alloc → map → fork → child-map → child-write → signal → unmap → free) confirmed | 1 wk    | 3-process (logic + render + sound) locked in as the baseline architecture |
| 3   | Sound child process ✅                    | [`sound.c`](../../src/c/sound.c) parent API + [`poc_sndc`](../../src/c/poc_sndc.c) child; SPSC `{freq,dur,amp}` ring buffer in shared memory; non-blocking from logic POV. Live-test passed 2026-04-25 | 0.5 wk  | Poll-based drain — signals interrupt SS.Tone |
| 4   | Render module / process ✅                | [`render.c`](../../src/c/render.c) parent API + [`poc_rndc`](../../src/c/poc_rndc.c) child; 64-entry SPSC RenderCmd ring; child owns CoVDG path + both 16K screens + page-flip. Live-test passed 2026-04-25; perf pass landed 2026-04-26 (signal-wakeup, byte-pair drawspr, byte-copy save_bg/rest_bg → 11 s startup, 27 fps) | 1–1.5 wk | `R_OP_PALETTE` deferred (SS.PalSet codepath unconfirmed) |
| 5   | Input — single keyboard ✅                | [`input.c`](../../src/c/input.c) module (poll + held/press/release edges) on `SS.KySns`, wired into `pioneer` as a SPACE-to-advance gate between phases (CTRL+SPACE skips to end). PoC `poc_input` confirmed bit pattern live on EOU 2026-04-26 (PR #39, #40). | 0.5 wk  | Polled from logic process — SS.KySns is non-blocking, no input child needed |
| ~~6~~ | ~~Minimal AI~~ — **dissolved 2026-04-26** | AI policies now ship inside their consuming 7x phase per [ai.md](../game/ai.md): land-grant in 7b, management in 7c/7d, auction in 7e/7f. 7a needs no AI. | — | GDD §18 ingested as the spec; no standalone AI module |
| 6.5 | **`poc_owtxt`** (gates 7a)                | Resolve the three open questions from the Windowing Manual Tier 1 ingest before any HUD/score code is written: (1) does `OWSet` accept the render child's CoVDG path, or do we need a parallel CoWin device window on the same screen? (2) does the `Palette`/`DefColr` escape sequence work on a CoVDG-displayed screen (potential simpler replacement for the deferred `R_OP_PALETTE` / `SS.PalSet` codepath)? (3) is the standard font preloaded under `pioneer`, or do we need explicit `GPLoad` + `Font` at startup? | 0.5 wk | See [cowin-text.md](../platform/cowin-text.md) "Open questions". Findings update [cowin-text.md](../platform/cowin-text.md), [cowin.md](../platform/cowin.md), and [lessons-learned.md](lessons-learned.md). |
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

**Critical path (1 → 2a → 2b → 3 → 4 → 5 → 6.5 → 7a..g → 8 → 9 → 10): ~12–13 effort-weeks** (phase 6 dissolved 2026-04-26; AI work absorbed into 7b/7c/7d/7e/7f).

## Dependency graph

```
1 ──► 2a ──► 2b ──► 4 ──┐
              │         │
              └──► 3 ───┤
                        │
                  5 ────┴──► 6.5 ──► 7a → 7b → 7c → 7d → 7e → 7f → 7g → 8 → 9 → 10
```

`poc_shmem` (2b) gates render path. Sound (3) and render (4) can run in parallel after 2b lands. `poc_owtxt` (6.5) gates 7a. AI policies land inside the 7x phase that consumes them — see [game/ai.md](../game/ai.md) for the per-phase split.

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
