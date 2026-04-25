# Source: NitrOS-9 EOU documentation

**Status:** STUBS — not yet deeply ingested. Very large (15K lines combined).

> **Note on accuracy:** The Tech Ref is hobbyist-written and contains rare errors. Cross-check against the NitrOS-9 assembly source at `D:\retro\nitros9\` (mirror of the upstream GitHub repo) when a behavior is load-bearing.

## Documents

| File | Lines | Covers |
|------|-------|--------|
| `docs/reference/NitrOS-9 EOU Operating System User's Guide.md` | 4416 | End-user shell/commands, OS operation |
| `docs/reference/NitrOS-9 EOU Technical Reference.md` | 9670 | System calls, drivers, VTIO/CoWin/CoVDG internals, VRN, IOMan |
| `docs/reference/NitrOS-9 EOU Level 2 Windowing System Manual.md` | 1385 | DWSet/OWSet/Select, escape sequences, GrfDrv, drawing commands |

## Anchor topics

### Technical Reference — primary target

Per AGENTS.md this is the authoritative source; reference-document-confirmed interfaces must be preferred before changing PoC code. Key lookups already made:

- **CoVDG `SS.AScrn` / `SS.DScrn` / `SS.FScrn`** — screen alloc, display, free. Called on a VTIO/CoVDG VDG path, not a CoWin window. `/verm` accepts; `term_vdg` / `TERM_VDG` did not in testing.
- **VRN on `/nil` via `SS.FSet`** — user-space 1/60s timing gate. Basis for `poc_vsync.c`. Documented in Tech Ref; not equivalent to direct GIME VBlank interrupts.
- **CoWin graphics commands** — all drawing via escape sequences or documented calls (DWSet, OWSet, SetDPtr, Line, Bar, FColor, BColor, Palette, GetBlk, PutBlk).
- **`SS.Tone`** — SetStat 0x98 on VTIO path; frequency 0–4095 relative, duration in ticks, amplitude 0–63. **Blocking** — see [platform/sound.md](../platform/sound.md).
- **`SS.Joy`** — GetStat for joystick, returns x/y/button. **`SS.KySns`** for real-time key sensing.
- **Memory & module-management syscalls** (ingested 2026-04-24 for IPC design — see [platform/ipc.md](../platform/ipc.md)):
  - `F$AllRAM` ($39) — allocate physical 8 KB RAM blocks. Confirmed by `level2/modules/kernel/fallram.asm`: bearer-style block IDs, no per-process ownership tracking (just a "used" mark in `D.BlkMap`).
  - `F$MapBlk` ($4F) — map block(s) by number into the *caller's* address space. No ownership check — anyone with a block number can map it.
  - `F$ClrBlk` ($50) — unmap blocks; counterpart to F$MapBlk for cleanup.
  - `F$Link` ($00) — find module in module directory by name + map into caller's space; refcount-managed.
  - `F$Load` ($01), `F$UnLink` ($02), `F$UnLoad` ($1D) — module lifecycle.
  - `F$VModul` ($2E) — validate a module header + CRC at a given DAT offset and **register it in the module directory**. Confirmed by `level1/modules/kernel/fvmodul.asm`: marks underlying physical blocks with `ModBlock` flag so they persist after the allocator exits. Enables runtime-built modules.
  - `F$Mem` ($07) — grow/shrink the calling process's data area.
  - `F$SRqMem` ($28) / `F$SRtMem` ($29) — system-pool memory; allocates from top of RAM. Pages-rounded.
  - `F$Move` ($38) — cross-task data move. Often privileged; primary use is system→user.
  - `F$CRC` — compute 24-bit module CRC. Useful when constructing a module header at runtime.
- **Process / signal syscalls** (used in stocks-and-bonds reference, confirmed against Tech Ref):
  - `F$Fork` ($03), `F$Wait` ($04), `F$Send` ($08), `F$Sleep` ($0A), `F$ID` ($0C). `F$Wait` returns `A=0` when interrupted by a signal received by the caller (vs. a child exit).

### Level 2 Windowing System Manual

- Screen types: type 2 (160×192×16, 16K), type 4 (320×192×16, 32K), type 6 (320×200×4), type 8 (320×200×16). See [platform/covdg.md](../platform/covdg.md) and [platform/cowin.md](../platform/cowin.md).
- CoWin "Select" escape `ESC $21` — required before CoVDG `SS.DScrn` output becomes visible on an opened VDG path.
- OWSet overlay windows for text-on-graphics.

### User's Guide

Lowest ingest priority; relevant only for command-line / shell interactions in dev workflow.

## Ingest strategy

Do **not** try to ingest all 15K lines. On demand, jump to the referenced section and expand the relevant platform page. For example: when PoC work needs a particular GetStat/SetStat, look up that one call and update [platform/](../platform/) rather than reading the whole driver chapter.
