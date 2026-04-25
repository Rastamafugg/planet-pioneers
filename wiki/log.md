# Wiki Log

Append-only chronological record of ingests, queries, and lints. Each entry prefixed `## [YYYY-MM-DD] <type> | <title>` so the log is grep-parseable.

---

## [2026-04-24] seed | Initial wiki instantiation

Created wiki scaffolding under `wiki/`:

- `CLAUDE.md` schema defining three-layer architecture (raw sources / wiki / schema), Obsidian-compatible relative-link conventions, and ingest/query/lint workflows.
- `index.md`, `log.md` root catalog + activity log.
- Source summary stubs in `sources/` for each of 9 raw source documents. GDD and AGENTS.md summarized substantively; all other references stubbed with pointers.
- Game-design pages seeded from `docs/design/MULE_GDD.md` sections 1–7, 11, 19–24: overview, modes, players, map, production, commodities, turn-sequence, management-phase, scoring. Auctions/events/economy/AI pages stubbed.
- Platform pages seeded from AGENTS.md lessons-learned and PoC source headers: stack, covdg, cowin, timing, sound, input, memory, dcc.
- Implementation pages: poc-catalog (all 7 PoCs), lessons-learned, data-structures, build-workflow, agent-workflow.

**Not yet ingested (stub pages only):**
- Full text of `mule-manual.md`, `mule-strategy-guide.md`, `Planet M.U.L.E. Rules.htm`
- All `docs/reference/*` DCC and NitrOS-9 reference manuals
- GDD sections 8–10, 12–18 (summary/land-grant/auction detail, production phase, random events, commodity auctions, economy, tournament features, AI)

Suggested next ingest: GDD §13 (Random Events) and §14 (Commodity Auctions) — these are required before implementation can begin on those phases, and current pages are stubs.

## [2026-04-24] ingest | stocks-and-bonds IPC reference

Ingested `D:\retro\stocks-and-bonds\src\c\SLPICPT.c`, `src\basic\TSTBUS4.b09`, `src\basic\TSTBUSW.b09` — a Basic09 sibling project on the same NitrOS-9 EOU platform that demonstrates multi-process architecture with C signal intercepts. Created [sources/stocks-and-bonds.md](sources/stocks-and-bonds.md) and new platform page [platform/ipc.md](platform/ipc.md) capturing: the `intercept()`-based signal handler pattern, the F$Fork/F$Wait/F$Send/F$Sleep/F$ID syscall set, and the "signal=wakeup, payload=shared state" discipline. Identified open question — cross-process shared memory (`F$AllRam`+`F$MapBlk`) — that gates whether Planet Pioneers can run render in a child process or must remain monoprocess for rendering.

## [2026-04-24] ingest | NitrOS-9 Tech Ref — memory & module-management syscalls

Targeted ingest of `docs/reference/NitrOS-9 EOU Technical Reference.md` for IPC design: F$AllRAM, F$MapBlk, F$ClrBlk, F$Link, F$Load, F$VModul, F$UnLink, F$UnLoad, F$Mem, F$SRqMem/F$SRtMem, F$Move, F$Wait, F$CRC. Cross-checked behavioral details against the kernel assembly source at `D:\retro\nitros9\` — specifically `level2/modules/kernel/fallram.asm`, `fmapblk.asm`, and `level1/modules/kernel/fvmodul.asm`. Resolved the two open questions blocking `poc_shmem`:

1. **`F$AllRAM` block IDs are bearer-style** — fallram.asm just sets a "used" mark in `D.BlkMap` with no per-process ownership table. Any process with the block number can `F$MapBlk` it. Caveat: blocks are not auto-freed on process exit; allocator (or reclaim path) must `F$ClrBlk`.
2. **Runtime module creation IS supported** — fvmodul.asm validates a module at a DAT-image offset, marks underlying blocks with `ModBlock` so they persist past the allocator, and adds an entry in the module directory for `F$Link` lookup.

Updated [sources/nitros9-docs.md](sources/nitros9-docs.md) with the syscall index, [platform/ipc.md](platform/ipc.md) (replaced hypothesis section with confirmed-fact section, added concrete `poc_shmem` design using F$AllRAM + F$MapBlk + block-number passing as baseline; F$VModul as upgrade path), and [platform/memory.md](platform/memory.md) (new "Cross-process shared memory" section). Updated [implementation/roadmap.md](implementation/roadmap.md): 3P architecture is now the working baseline (2P fallback contingent only on DCC/libc obstacles), and `poc_shmem` shifts from "decide kernel feasibility" to "validate C-side implementation."

## [2026-04-24] decision | Shared-memory mechanism — confirmed vs hypothesized

User confirmed file-based IPC with full-file and section locking works on EOU — recorded as the guaranteed fallback transport in [platform/ipc.md](platform/ipc.md). User raised the hypothesis that a parent process might construct a data module at runtime so children can `F$Link` to it; recorded as the preferred in-memory candidate pending Tech Ref confirmation. Updated ipc.md with an explicit confirmed-vs-hypothesized split. Updated [implementation/poc-catalog.md](implementation/poc-catalog.md) "next PoC" line to reflect new ordering: poc_ipc → poc_shmem → sound child. **Next concrete step:** targeted ingest of NitrOS-9 EOU Technical Reference sections F$AllRam, F$MapBlk, F$Link, F$LdMod, F$Move, F$Mem before `poc_shmem` is designed.

## [2026-04-25] finding | `direct` storage class crashes EOU

Running `pp` on the CoCo 3 with `direct GameState g_state` produced a system crash (graphic bars down the screen) on the first `printf`. Hypothesis: DCC's `direct` allocator overlaps libc's own direct-page slots (`_flacc`, `errno`, ...; visible in stocks-and-bonds [SLPICPT.c](../../stocks-and-bonds/src/c/SLPICPT.c)). No PoC in `src/c/` actually uses `direct` — the GDD §23.3 recommendation was design intent, not observed practice.

Reverted [main.c](../src/c/main.c) to plain `GameState g_state` in the data segment. Marked `direct` as ⚠ unverified in [platform/memory.md](platform/memory.md) and [platform/dcc.md](platform/dcc.md). Recorded as observed fact in [implementation/lessons-learned.md](implementation/lessons-learned.md). A dedicated direct-page PoC is now a prerequisite before any production code uses `direct`.

## [2026-04-25] lint | DCC build findings from phase 1/2a

Two observed-fact findings recorded in [implementation/lessons-learned.md](implementation/lessons-learned.md) after first EOU build attempt:

1. **DCC linker collides on long shared name prefixes.** `phase_land_grant` vs `phase_land_auction` triggered `multiple definition` at [main.c](../src/c/main.c:56). Renamed all phase functions to short distinct `ph_xxx()` and marked them `static` (PR #5).
2. **`<os9.h>` already defines `F_ID`, `F_FORK`, `F_WAIT`, `F_SEND`, `F_SLEEP`.** Bare `#define` produces `redefined macro` warnings. Wrapped the duplicates in [poc_ipc.c](../src/c/poc_ipc.c) and [poc_ipcc.c](../src/c/poc_ipcc.c) with `#ifndef` guards per [poc_vsync.c](../src/c/poc_vsync.c) pattern.

## [2026-04-24] implement | Phase 1 core skeleton + Phase 2a poc_ipc

Implemented roadmap phases 1 and 2a per [implementation/roadmap.md](implementation/roadmap.md).

- **Phase 1** — `src/c/main.c`: `direct GameState g_state` (6 B), phase enum (`SUMMARY → LAND_GRANT → LAND_AUCTION → RANDOM_EVENT → MANAGEMENT → PRODUCTION → AUCTION`), `next_phase()` round counter, stub phase functions printing their name. Builds as `/dd/cmds/pp` with `-m=4k` (no `-s`).
- **Phase 2a** — `src/c/slpicpt.c` (verbatim port of stocks-and-bonds `SLPICPT.c` minus the standalone `_stkcheck`/`vsect` block), `src/c/poc_ipc.c` (parent: F$ID → install intercept → F$Fork "pocipcc" with parent PID as `argv[1]` → poll for sig 130 → F$Wait), `src/c/poc_ipcc.c` (child: `atoi(argv[1])` → F$Send 130 → exit). F$Fork uses A=0/B=namelen/X=name(last char OR `$80`)/Y=paramlen/U=param per `TSTBUS4.b09` convention.

Resolved [platform/ipc.md](platform/ipc.md) open question #3 (canonical fork path = direct `_os9(F$Fork)`, not `system()`). Reserved signal 130 = `SIG_IPC_ACK` (PoC-only); full table still TBD.

Build scripts: `src/script/buildc` extended with three new dcc lines; `src/script/patchc` rewritten to the same three for current iteration. Updated [implementation/poc-catalog.md](implementation/poc-catalog.md).

QA pending: live build + run on EOU. If F$Fork-from-DCC misbehaves, escalation per design is to fall back to `system()`.

## [2026-04-24] plan | Implementation roadmap committed

Created [implementation/roadmap.md](implementation/roadmap.md) — phase plan from current state (post double-buffer verification on `poc_cvdg16`) through full game + QA. ~13–14 effort-weeks on the critical path. Architectural choices captured: multi-process baseline (logic + render + sound) with 2P fallback if `poc_shmem` fails; single-human keyboard scope first with joystick/multi-keyboard as polish; AI required from first playable phase. Two new gating PoCs added to the work list: `poc_ipc` (port SLPICPT) and `poc_shmem` (cross-process shared memory). Linked from index under both Platform and Implementation sections.
