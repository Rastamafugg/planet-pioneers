# Wiki Index

Content catalog. Read this first when answering a query to find the right page, then drill in.

## Game design

- [Overview](game/overview.md) — what M.U.L.E. is, game length, win condition
- [Difficulty modes](game/modes.md) — Beginner / Standard / Tournament rule differences
- [Players and characters](game/players.md) — 1–4 players, 8 species in 3 economic tiers
- [Planet map](game/map.md) — 5×9 grid, 44 ownable plots, store column
- [Terrain and production](game/production.md) — base production, EOS/LCT bonuses, variance models
- [Commodities](game/commodities.md) — Food / Energy / Smithore / Crystite
- [Turn sequence](game/turn-sequence.md) — phases per round
- [Colony management phase](game/management-phase.md) — the iconic MULE-walking phase
- [Auctions](game/auctions.md) — land auction + commodity auctions (stub)
- [Random events](game/random-events.md) — player events, planetary events (stub)
- [Economic model](game/economy.md) — store prices, supply/demand (stub)
- [Scoring and victory](game/scoring.md) — net worth formula, colony survival threshold
- [Computer player AI](game/ai.md) — AI behavior targets (stub)

## Platform / technology

- [Platform stack](platform/stack.md) — CoCo 3 + NitrOS-9 EOU + DCC overview
- [CoVDG graphics](platform/covdg.md) — VDG screen types, page flipping, SS.AScrn/DScrn/FScrn
- [CoWin graphics](platform/cowin.md) — windowing system, DWSet, drawing commands, GET/PUT
- [Timing and VSync](platform/timing.md) — 60Hz ticks, F$Time, VRN /nil VIRQ gate
- [Sound](platform/sound.md) — SS.Tone, blocking behavior, dedicated-process pattern
- [Input](platform/input.md) — joystick (SS.Joy), keyboard (SS.KySns)
- [Memory layout](platform/memory.md) — process address space, direct page, GrfDrv
- [DCC compiler notes](platform/dcc.md) — K&R dialect, constraints, flags

## Implementation

- [PoC catalog](implementation/poc-catalog.md) — all seven `src/c/poc_*.c` files and what each proves
- [Lessons learned](implementation/lessons-learned.md) — observed-fact findings from PoCs
- [Data structures](implementation/data-structures.md) — Plot / Player / Store / GameState / Auction
- [Build workflow](implementation/build-workflow.md) — `buildc` / `patchc` / disk image discipline
- [Agent workflow](implementation/agent-workflow.md) — roles defined in AGENTS.md (PM → BA/CA/Debugger → Impl → QA)

## Sources

- [MULE Game Design Document](sources/gdd.md) — primary source of truth; 1654 lines across 24 sections
- [AGENTS.md](sources/agents-md.md) — project rules, roles, constraints, PoC lessons
- [M.U.L.E. manual (prose)](sources/mule-manual.md) — original 1983 manual (stub)
- [M.U.L.E. strategy guide](sources/strategy-guide.md) — strategy tips (stub)
- [Planet M.U.L.E. rules](sources/planet-mule-rules.md) — the "Option B" source (stub)
- [PoC source files](sources/poc-sources.md) — index of `src/c/poc_*.c`
- [DCC C Compiler docs](sources/dcc-docs.md) — library ref + user's guide (stubs)
- [NitrOS-9 EOU docs](sources/nitros9-docs.md) — user's guide, technical ref, windowing manual (stubs)

## Special

- [CLAUDE.md](CLAUDE.md) — wiki schema and workflows
- [log.md](log.md) — chronological activity log
