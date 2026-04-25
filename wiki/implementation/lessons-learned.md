# Lessons Learned

Observed-fact findings from PoC work. Mirrors AGENTS.md §66+ with links to the wiki pages that act on each finding. Do **not** remove items here without replacing with a clear "superseded by X" note.

## DCC / language ([dcc.md](../platform/dcc.md))

- **K&R dialect only.** Declarations at start of block; no `void` prototypes; no modern C syntax. Keep external symbol names short and distinct to reduce linker/name-collision risk.
- **16-bit `int`.** Plan data sizes accordingly.
- **Linker collides on long shared name prefixes.** `phase_land_grant` and `phase_land_auction` produced `multiple definition` at link time — even with full names that differ in characters 11+. Mitigation: keep the distinct portion of an external name within the first ~6 chars, and mark file-local helpers `static`. Observed 2026-04-25 building [main.c](../../src/c/main.c).
- **NitrOS-9 ABI is size-based, not type-based.** Both Basic09 and DCC C pass parameters by *size in bytes*, with no automatic widening of `char` (1 B) to `int` (2 B) at varargs / unprototyped call sites. Passing `unsigned char` directly to `printf("%d", ...)` makes printf read 2 bytes per `%d`, get stack garbage in the high byte, format an out-of-range integer, and the terminal interprets the resulting high-bit / control bytes as block-graphic glyphs ("graphic bars down the screen"). **Mitigation:** prefer `int` for any value that crosses a function boundary; if a struct field must be `char` for size, widen with `(int)` at every call site. Confirmed by user 2026-04-25 ("known aspect of coding for NitrOS-9 using Basic09").
- **`direct` storage class — status uncertain.** Initial crash of `pp` was provisionally blamed on `direct GameState g_state` (hypothesis: collision with libc's direct-page slots `_flacc`, `errno`). Removing `direct` did **not** fix the crash — the real cause was the printf-promotion bug above. The `direct` mechanism therefore remains untested rather than refuted; treat as a future PoC target. No existing PoC in `src/c/` uses it.
- **`<os9.h>` already defines the syscall numbers.** `F_ID`, `F_FORK`, `F_WAIT`, `F_SEND`, `F_SLEEP` (and likely the rest) ship in DCC's `os9.h`. Plain `#define` redeclaration produces `redefined macro` warnings — guard with `#ifndef` per the [poc_vsync.c](../../src/c/poc_vsync.c) pattern. Observed 2026-04-25 building [poc_ipc.c](../../src/c/poc_ipc.c).

## [CoWin](../platform/cowin.md) GET/PUT sprite movement

- `GetBlk`/`PutBlk` **are fast enough** for small sprite tests when the buffer contains the **union of restored background and new sprite**.
- **One 40×40 union buffer per movement direction** reduced flicker better than separate background restore + sprite put.
- Use **byte-aligned X coordinates** where possible; 8-pixel steps succeeded in testing.
- **CoWin command buffering does NOT provide true page flipping or guaranteed VBlank synchronization.**

## [Timing](../platform/timing.md)

- **VRN `/nil` `SS.FSet`** provides a documented 1/60s timing gate — good for measurement and frame pacing, **not** for tear-free hardware-tight vsync.
- **`F$Time` returns only whole seconds** — FPS/timing output is coarse. If a test reports unexpectedly slow timing, treat that as observed fact and run a discriminating test before assuming a cause.

## [CoVDG](../platform/covdg.md) screens

- Call `SS.AScrn`, `SS.DScrn`, `SS.FScrn` with `I$SetStt` on a **VTIO/CoVDG VDG path**, not a CoWin window.
- `/verm` has been observed to accept `SS.AScrn` where `term_vdg` and `TERM_VDG` did not.
- `SS.DScrn` only displays if the CoVDG path is the **current interactive device**.
- Send CoVDG `Select` (`ESC $21`) to the opened VDG path **before** expecting `SS.DScrn` output to become visible.
- Screen 0 should be **displayed before freeing** allocated high-resolution screens.

## CoVDG memory

- **320×192×16 CoVDG screen type 4** requires 32 KB; a **second** 32K allocation failed with err #207 in testing.
- **160×192×16 CoVDG screen type 2** requires 16 KB and is the **viable page-flip target**.

## [Sound](../platform/sound.md)

- **`SS.Tone` is BLOCKING** — process pauses for the full tone duration.
- Production architecture **requires a dedicated sound process** reading `{freq, dur, amp}` tuples from a pipe.
- Frequency is a **relative counter 0–4095**, not Hz. Widest pitch variation at the high range; PoC intentionally uses high values.

## Workflow

- Leave repository files as source of truth; the deploy/build workflow owns `disks/ppsrc.dsk`.
- Prefer **reference-document-confirmed interfaces** before changing PoC code. Primary sources: NitrOS-9 Technical Reference, Level 2 Windowing System Manual, DCC references.

## Sources

- `AGENTS.md` §66–94
- PoC file headers in `src/c/`
