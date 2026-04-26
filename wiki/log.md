# Wiki Log

Append-only chronological record of ingests, queries, and lints. Each entry prefixed `## [YYYY-MM-DD] <type> | <title>` so the log is grep-parseable.

---

## [2026-04-26] perf | drawspr byte-pair fast path

After PR #36's byte-copy save_bg landed, `drawspr` was the largest remaining hot spot — 64 per-pixel `putpx` (mask+or+store) per sprite × 2 sprites per frame. Rewrote drawspr to process two source pixels at a time, dispatching on their joint opacity: both opaque collapses to a single byte store, both transparent to a skip, mixed to one read-modify-write. ~3× faster than the per-pixel form. New lesson: any pixel-level loop with byte-aligned start can be rewritten as a byte loop with at most 4 cases.

---

## [2026-04-26] perf | Phase 4 byte-copy save_bg/rest_bg

Signal wakeup (PR #35) did not improve the 11 fps; sleep latency wasn't the bottleneck. The real cost was `paint_bg_at` + `tile_color` doing 64 putpx + 64 range-check chains per slot per frame, ~5x more expensive than poc_cvdg16's byte-copy save_bg. Reverted to byte-copy save_bg/rest_bg, keeping correctness under overlap by relying on the three-pass-at-present pipeline introduced in PR #33: all clears (rest_bg) run first, then all saves (save_bg) capture clean post-clear screen, then all draws — no save can ever capture another sprite's pixels. New lessons in [lessons-learned.md](../wiki/implementation/lessons-learned.md): measure before optimizing the wait; per-pixel `putpx` is the dominant cost on this platform.

---

## [2026-04-26] perf | Phase 4 signal-based wakeup

After PR #34: animation correct, FPS at 11. Bottleneck was the 1-tick (~16ms) `F$Sleep(1)` floor on both ends of the IPC. Replaced with signal-based wakeup: parent sends `SIG_RENDER_WAKE` (133) after enqueueing, child sends `SIG_RENDER_DONE` (130) after draining. Both ends sleep with `F$Sleep(0)` + `intercept()` handler. Render-specific syscalls (`SS.DScrn`, byte writes) aren't signal-interruptible like `SS.Tone`, so the sound-child's "must use poll" rule doesn't apply here. New field `parent_pid` in RenderQueue header lets the child know where to send DONE. `ren_shut`'s `F$Wait` now loops past spurious A=0 returns from signal interruption. New lessons recorded in [lessons-learned.md](../wiki/implementation/lessons-learned.md). Signal numbers documented as a table in [ipc.md](../wiki/platform/ipc.md).

---

## [2026-04-26] fix | Phase 4 sprite overlap residue + frame-rate

Live-test feedback after #32: (a) animation correct but a few frames during sprite overlap showed pixels from the first-drawn sprite replaced with map background; (b) steady-state animation only ~5 fps vs poc_cvdg16's ~30 fps. Fixes:
1. `R_OP_SPRITE` no longer draws inline — records into a per-slot pending struct. `R_OP_PRESENT` runs two passes (all bg-clears, then all draws) before SS.DScrn. Sprite ordering can no longer cause cross-erasure under overlap.
2. `paint_bg_at` divisions hoisted out of the inner loop — `(col, tx, row, ty)` computed once per sprite footprint with a tiny while-loop instead of 64 expensive `int` divides. K&R DCC int division is hundreds of cycles each; the naive form dominated the per-frame budget. Two new lessons recorded in [lessons-learned.md](../wiki/implementation/lessons-learned.md).

---

## [2026-04-26] fix | Phase 4 startup, dmap visibility, sprite overlap

Live-test feedback after #31: (1) ~30 sec startup; (2) tiles painted in line-by-line on the visible screen during dmap; (3) sprite overlap left "black gap" residual. Fixes:
1. `hline` rewritten with a byte-fill fast path (whole-byte `*p++ = fill` loop) — full-screen clear and tile fills run roughly an order of magnitude faster.
2. `R_OP_DRAWMAP` now memcopies the back buffer to the other screen after drawing, so one call seeds both screens (one 16K copy ≪ second procedural pass).
3. `g_back` initialized to 1 (not 0) so the first dmap draws to the offscreen buffer; user no longer sees piecemeal tile painting on the visible screen.
4. `save_bg` / `rest_bg` removed entirely. New `tile_color(kind, tx, ty)` query (mirrors plain/river/mountn rect calls) + `paint_bg_at(base, x, y)` derive the 8×8 sprite-footprint clear directly from `g_map`. Sprites can overlap in any order without ghosting.
Public API change: smoke driver now calls `ren_dmap()` once instead of twice. Lessons recorded in [lessons-learned.md](../wiki/implementation/lessons-learned.md): screen-sampled save_bg is unsafe; first-frame `g_back` must point offscreen; dmap should clone, not redraw.

---

## [2026-04-26] perf | Phase 4 render save_bg/rest_bg port

Second live test of `pocrnd` ran the animation but at ~1 frame every few seconds. Diagnosis: smoke driver enqueued full redraw per frame (clear + 45 tiles + 2 sprites = ~24K putpx). Fix: ported `poc_cvdg16`'s save_bg/rest_bg pattern into the render child with per-(screen, slot) bg buffers. New ops `R_OP_DRAWMAP` (called twice at startup to seed both screens) and `R_OP_SPRITE` reworked to take a slot index and do bg restore-save-draw internally. `ren_spr` API gained a `slot` parameter; `dir`+`mule` packed into one byte to keep RenderCmd at 8 bytes. New public symbol `ren_dmap`. Per-frame command count dropped from 49 to 3 (2 sprite + present). New lesson recorded: logic enqueues deltas, child owns persistence; page-flip + save_bg requires per-screen bg buffers.

---

## [2026-04-25] fix | Phase 4 render init timing

First live test failed: `pocrnd` reported `render: child not ready after 5s` and exited with err 4. Diagnosis: child performed two software full-screen rect-clears + `vsel` + `show_screen` BEFORE setting `q->ready = 1`; on emulators paced below real CoCo speed this exceeded the parent's 5-sec init budget. Fix: move `q->ready = 1` to immediately after both screens are allocated; defer cosmetic prep (vsel/clear/show) to after the handshake — the first drained command is `ren_clr()` which redraws anyway. Bumped parent timeout 5s→30s defense in depth. Bumped child `-m=4k`→`-m=8k` for stack headroom in deep `draw_tile` call chains. New lesson recorded in [lessons-learned.md](../wiki/implementation/lessons-learned.md): child `ready` flag gates only the minimum the parent needs.

---

## [2026-04-25] phase | Phase 4 render child — code landed

Wrote [src/c/render.c](../src/c/render.c) (parent API), [src/c/poc_rndc.c](../src/c/poc_rndc.c) (child), [src/c/poc_rnd.c](../src/c/poc_rnd.c) (smoke driver). Architecture mirrors phase-3 sound: SPSC ring (64 × 8-byte `RenderCmd`) in an 8K F$AllRAM block, poll-based wakeup with `F$Sleep(1)`, no signals. Render child owns the CoVDG path + both 16K screens + page-flip; logic enqueues clear/tile/sprite/present intents and calls `ren_flush()` to sync. Public symbols use `ren_` prefix to dodge the DCC 8-char external-name collision (`render_present` vs `render_palette`, `render_sprite` vs `render_shutdown`). `patchc` and `buildc` updated. `R_OP_PALETTE` accepted but child handler is a stub — `SS.PalSet` codepath deferred to first phase needing custom colors. Awaiting live-test on EOU.

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

## [2026-04-25] decision | Phase 3 done; signals abort SS.Tone

`pocsnd` live-test passed: four queued tones played in sequence while the parent reached its shutdown print before the playback finished — fire-and-forget confirmed.

Two findings recorded in [implementation/lessons-learned.md](implementation/lessons-learned.md) along the way:
1. `/term` must be opened mode 3 (r/w) for `SS.Tone`; mode 1 fails silently.
2. **`SS.Tone` is interruptible by signals** — a process that produces tones cannot use `F$Send` as its wake mechanism, because the producer's signals abort the consumer's tones inaudibly. Sound child now uses a short `F$Sleep(2 ticks ≈ 33 ms)` poll instead of signal-wake. This is a notable architectural constraint: signals + shared memory works fine for *most* IPC, but the sound subsystem specifically needs poll-based wake.

[implementation/roadmap.md](implementation/roadmap.md): phase 3 ✅. Next: phase 4 — render module (promote `poc_cvdg16` into a render child).

## [2026-04-25] implement | Phase 3 sound child (C-side; live-test pending)

Composed phases 2a (signals) and 2b (shared memory) into the sound subsystem.

- [src/c/sound.c](../src/c/sound.c) — parent-side API: `sound_init()` (alloc + map + fork `pocsndc`), `sound_play(freq,dur,amp)` (enqueue + signal), `sound_shutdown()` (`quit=1` + signal + F$Wait + F$ClrBlk + F$DelRAM).
- [src/c/poc_sndc.c](../src/c/poc_sndc.c) — child: maps shared SoundQueue from `argv[1]`, opens `/term`, signal-driven drain loop calling SS.Tone per entry. Drains pending entries before honouring `quit`.
- [src/c/poc_snd.c](../src/c/poc_snd.c) — smoke driver: bring up, queue 4 tones, shut down.

Shared queue layout: 16-entry SPSC ring of `{freq,dur,amp}` 16-bit triples + `magic`/`head`/`tail`/`quit` header (104 B total of an 8 KB block). Producer writes entry then advances `head`; consumer reads entry then advances `tail`. Atomic 16-bit accesses on 6809 mean no locks needed for the single-producer-single-consumer case.

Reserved `132 = SIG_SOUND_WAKE`.

[implementation/poc-catalog.md](implementation/poc-catalog.md) updated; phase 3 in [implementation/roadmap.md](implementation/roadmap.md) marked ⏳ live-test pending.

## [2026-04-25] decision | Phase 2b done; 3-process architecture locked in

`pocshm` live-test passed on EOU end-to-end:
1. `F$AllRAM` allocated one 8 KB physical block
2. `F$MapBlk` mapped it into the parent's DAT image; magic + counter init OK
3. `F$Fork pocshmc` with `"<block#> <pid>\r"` in argv
4. Child mapped the **same physical block** (different logical addr in its own DAT image), verified magic, incremented counter
5. Child sent SIG_SHMEM_ACK, parent received it, **read counter == 2** confirming the bytes were shared
6. `F$Wait` reaped child, `F$ClrBlk` unmapped, `F$DelRAM` freed physical block

This was the gating PoC for the multi-process architecture. **3-process baseline (logic + render + sound) is now locked in.** 2P fallback no longer in play.

[implementation/roadmap.md](implementation/roadmap.md) — phase 2b ✅; critical-path risk #1 (cross-process render shmem) struck through.

Next gating PoC: phase 3 — sound child process.

## [2026-04-25] implement | Phase 2b poc_shmem (C-side; live-test pending)

Implemented [poc_shmem.c](../src/c/poc_shmem.c) (parent) and [poc_shmemc.c](../src/c/poc_shmemc.c) (child) using the design from [platform/ipc.md](platform/ipc.md). Parent allocates one 8 KB block via `F$AllRAM`, maps it into its DAT image via `F$MapBlk`, writes a `0x5AA5` magic + counter `1`, forks `pocshmc` with `"<block#> <pid>\r"` in argv, polls for `SIG_SHMEM_ACK` (131). Child maps the same physical block, verifies magic, increments counter to 2, sends signal, unmaps, exits. Parent verifies counter, reaps via `F$Wait`, unmaps with `F$ClrBlk`, frees physical block with `F$DelRAM`.

While verifying register conventions against the kernel source, found a wiki conflation: `F$ClrBlk` only unmaps from the caller's DAT image; `F$DelRAM` is the call that actually deallocates the physical block (clears `RAMinUse` in `D.BlkMap`). Updated [platform/memory.md](platform/memory.md) and [platform/ipc.md](platform/ipc.md) with the corrected lifecycle.

[implementation/poc-catalog.md](implementation/poc-catalog.md) updated; phase 2b in [implementation/roadmap.md](implementation/roadmap.md) marked ⏳ live-test pending. Phase 2b decides 3-process (logic + render + sound) vs 2-process architecture for the rest of the project — success here locks in the 3-process baseline.

## [2026-04-25] decision | Phase 2a done

`pocipc` live-test passed on EOU. Parent successfully:
1. `F$ID` for own PID
2. installed signal intercept via `slpicpt`
3. `F$Fork`-ed `pocipcc` with parent PID in argv
4. polled and received `SIG_IPC_ACK` (130) from the child
5. `F$Wait`-reaped the child cleanly

Resolved [platform/ipc.md](platform/ipc.md) open question #3 (canonical fork path = direct `_os9(F$Fork)`, not libc `system()`). Resolved one new lesson during the live test: `#asm ldy 6,s` preamble in `slpicpt.c` (left over from its standalone-module Basic09 invocation in stocks-and-bonds) clobbers Y when linked into a C program — see [implementation/lessons-learned.md](implementation/lessons-learned.md).

Phase 2a marked ✅ in [implementation/roadmap.md](implementation/roadmap.md). Next gating PoC: `poc_shmem` (phase 2b).

## [2026-04-25] decision | Phase 1 done

`pioneer` (the phase-1 core skeleton, [src/c/main.c](../src/c/main.c)) runs cleanly: 1 init line + 42 phase lines (6 rounds × 7 phases) + final summary. Canonical shape: module name `pioneer`, seven `static ph_xxx()` helper functions dispatched from a `switch`, `unsigned char` GameState fields with `(int)` casts at every printf site (per data-structures.md and the size-not-type ABI rule).

Phase 1 marked ✅ in [implementation/roadmap.md](implementation/roadmap.md). Phase 2a (`poc_ipc`) is built but live-test deferred.

The original phase-1 crashes (graphic bars / vertical stripes / system reset) were **not isolated to a specific source-level cause** — bisects A/B/C reverting field type, helper functions, and module name individually all ran clean. New observation captured in [implementation/lessons-learned.md](implementation/lessons-learned.md): treat a NitrOS-9 module crash as something that may persist past its source fix, and rebuild from `buildc` (not `patchc`) before bisecting.

## [2026-04-25] finding | DCC does not auto-promote char→int at printf call sites

Running `pp` crashed EOU twice with the same symptom (graphic bars down the screen). Initially blamed `direct GameState g_state`; removing `direct` did not fix it. Real cause: every printf in `main.c` passed `unsigned char` fields (`g_state.mode`, `g_state.round`, ...) directly to `%d`. DCC's K&R varargs ABI does not appear to promote `char` to `int` reliably — printf reads 2 bytes per `%d`, gets stack garbage in the high byte, formats it, and the terminal interprets the resulting high-bit/control bytes as block-graphics glyphs that fill the screen.

Cross-checked: every working PoC's printf passes `int` (`vcnt`, `r.rg_b & 0xff`, ...). main.c was the first to violate that. Fixed by adding `(int)` casts at every printf site. Recorded in [implementation/lessons-learned.md](implementation/lessons-learned.md), updated [platform/dcc.md](platform/dcc.md) and [platform/memory.md](platform/memory.md). The `direct` keyword is now correctly classified as **unverified** rather than known-broken — needs its own PoC.

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

## [2026-04-25] ingest | DCC C Compiler User's Guide — Tier 1 quirks pass

Targeted ingest of `docs/reference/DCC C Compiler System Users Guide.md` (3685 lines) focused on **what trips up a modern C programmer**. Read in full: Ch 2 "DCC Characteristics" (lines 387–649), Appendix C §§ C.2 / C.6 / C.7 / C.8 / C.10–C.18 (lines 1989–3617), Appendix D "DCC vs Microware C" (lines 3618–3677). Skipped: Ch 1 tutorial, Ch 4 Basic09 examples, Ch 5 RMA reference, Appendix A error messages.

Updated [sources/dcc-docs.md](sources/dcc-docs.md) from stub to full source-summary with section-to-line map, so future targeted reads can jump straight in. Substantially rewrote [platform/dcc.md](platform/dcc.md): added Type System (sizes, custom non-IEEE float/double layout, `pflinit`/`pffinit` referencing dance), Storage Classes (full `direct` documentation — 255-byte limit, libc-disjoint, no function args, K&R grammar omits it), ABI (spec vs implementation contradiction on call-site promotion), Preprocessor (Microware-C ANSI retrofits — stringizing/`__FILE__`/`#warning`/`#error`/`_OS9`/`__mc6809__`/`_BIG_END`), Run-time Arithmetic Errors (`EFPOVR`/`EDIVERR`/`EINTERR` via signal), and a complete compile-flag table with `-O`/`-P`/`-S`/`-M`/`-m=`/`-l=` semantics.

Appended 12 non-obvious gotchas to [implementation/lessons-learned.md](implementation/lessons-learned.md). Two flagged contradictions:

1. **Identifier significance is documented at exactly 8 characters** (UG line 2013), refining the prior empirical "first ~6 chars" rule. Confirms the `phase_land_grant` / `phase_land_auction` collision (shared `phase_la` first-8). Updated rule: distinct portion within first 8 characters of any external (non-`static`) name.
2. **Call-site `char`→`int` promotion**: spec (UG § C.7.1 line 2216 and § C.10.1 line 3039) claims it happens; our printf-graphic-bars finding shows it doesn't for varargs. Tentative resolution recorded — promotion requires a formal parameter to drive it, so `…` varargs and unprototyped calls fall through to size-not-type ABI.

Other gotchas surfaced: `\n` is CR not LF (use `\l` for actual LF); auto aggregates / unions cannot be initialized; one `register` per function silently downgraded; string literals not pooled; all float arithmetic widens to double; float/double layout is sign+magnitude+biased-exp, not IEEE 754; no `void` and no `enum` keywords; `direct` absent from Appendix C grammar (Ch 2 extension); `#asm` blocks in `vsect` need `endsect` before `#endasm`; `<…>` includes search DEFS only.

No new pages created; [index.md](index.md) unchanged. Library Reference still stub — read on demand for syscall / libc specifics.

## [2026-04-25] ingest | DCC User's Guide — Tier 2 ABI / module / toolchain

Targeted ingest of `docs/reference/DCC C Compiler System Users Guide.md` Ch 2 § Compiler option flags + Examples (lines 616–649), Ch 3 §§ Object code module + Memory management (650–830), and Appendix B per-tool flag reference for `dcc`/`dcpp`/`dcc68`/`dco68`/`rma`/`rlink` (1863–1988).

**Headline finding — correction of long-standing wiki claim:** `-s` is documented (Ch 2 line 631; Appendix B lines 1903 and 1935) as "suppress generation of stack-checking code," NOT "optimize for speed." Prior wiki, AGENTS.md, and the GDD's recommended flags treated `-s` as a speed flag. PoCs running with `-s` therefore have no `*** Stack Overflow ***` runtime detection — a stack-into-data collision becomes silent corruption. Correction applied in [platform/dcc.md](platform/dcc.md) and a strong-flag entry added to [implementation/lessons-learned.md](implementation/lessons-learned.md). **Recommendation:** drop `-s` from PoC build lines until each PoC's stack high-water has been measured.

**Other notable findings:**

- **`-2` (Level-2 optimization) is documented as actively dangerous** ("extremely likely to fail, may even crash the computer!" — App. B line 1883). Our target IS Level 2, but `dco68` already applies safe Level-2 patterns automatically (Appendix D line 3668, Level-2 zero-base absolute addressing). Don't enable `-2` manually.
- **`-ls` links `sys.l`** — a library of OS-9 syscall constants, `I$GetStt`/`I$SetStt` codes, and SS_* constants (App. B line 1894). PoCs currently `#define` these manually with `#ifndef` guards; `-ls` is the documented alternative. Worth a small verification PoC.
- **`-lg` links `cgfx.l`** — a Tandy CoCo 3 graphics library (App. B line 1892). Existence not previously known to the project. Scope unknown — may overlap CoVDG/CoWin work.
- **String literals in DCC live in the TEXT section of the OS-9 module, not DATA** (UG line 713). Mutating `*p` after `char *p = "..."` corrupts the executable module image — which is reentrant and potentially shared. Use `char arr[] = "..."` for mutable strings.
- **Per-function stack overhead** (UG lines 778 + 825): +64 bytes reserved for asm/syscall/arithmetic + 4 bytes for return address & caller's register-var save, on top of declared locals.
- **stdio buffer cost** (UG line 827): 256 bytes per `fopen`'d file; `stderr` is unbuffered. Significant for `-m=` budgeting.
- **`dcpp` derives psect names from filename + `_c`** (App. B line 1913) — two source files with the same basename in different directories produce a link-time clash distinct from the 8-char-identifier rule.
- Module format documented end-to-end (Type/Lang $11, Attr/Rev $81 reentrant, mainline = `cstart.r`, data-text/data-data reference tables drive runtime pointer relocation), including Level-2 specifics (Y=0, DP=0, data area starts at zero).

Distinguished `-O` (capital — stop after optimizer) from `-o` (lowercase — inhibit optimizer); distinguished `-m=` (dcc-level additional memory) from `-M=` (rlink-level total data area) from `-M` (no `=` — request linkage map). Full flag table now in [platform/dcc.md](platform/dcc.md).

No new pages created; [index.md](index.md) unchanged. Source-summary [sources/dcc-docs.md](sources/dcc-docs.md) updated with Tier 2 ingest entry. Tier 3 (Ch 4 Basic09 ABI, Ch 2 perf section, App. C expressions skim) remains optional — flag if a specific question motivates it.

## [2026-04-26] fix | dead paint_bg_at causes pocrndc link error

User hit `Unresolved references: tile_col   poc_rndc_c   in ctmp.4.r` after testing the latest phase 4 changes. Diagnosis: PR #36 reverted to byte-copy save_bg/rest_bg and removed the `tile_color` definition, but left `paint_bg_at` (its only caller) in place. With no callers of its own and only a now-missing dependency, `paint_bg_at` was dead code dragging an unresolved reference. Deleted the function and updated the stale comment over `g_bg`. patchc trimmed to just `pocrndc`.

Lesson: post-revert dead-code sweeps need to follow the call graph one hop further than the immediate change set. (Not added to lessons-learned — single occurrence, generic dead-code hygiene rather than a NitrOS-9 quirk.)

## [2026-04-26] milestone | phase 4 complete

User confirmed: test run in 11 s startup, 27 fps sustained. Phase 4 (multi-process render child with shared queue, byte-copy save_bg/rest_bg, drawspr byte-pair fast path, signal-based wakeup, dead-code cleanup) is closed.
