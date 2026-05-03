# Memory Layout

## Process model

NitrOS-9 EOU Level 2 gives each process a **single 64 KB logical address space**, mapped from 512 KB physical RAM via an 8 KB-page MMU. The MMU itself is part of the GIME chip — eight 6-bit Page Address Registers (PARs) per set, executive set for the kernel, task set for the process. See [gime.md](gime.md#mmu-chapter-3) for the hardware view. Code + data + stack must fit within this 64 KB window (though the **GrfDrv**-managed graphics memory lives in the system map and does not count against process space).

## DCC layout

- **Code segment** — program code
- **Data segment (Y-relative)** — DCC places globals and statics here
- **Stack** — sized via `-m=` flag (e.g. `-m=4k`, `-m=8k`, `-m=32k`)
- **Direct page** — 255 bytes; access by `direct` storage class

## Direct page discipline

255 bytes total. Per GDD §23.3 the design intent is to place `GameState g_state` and hot-path variables here:

```c
direct GameState g_state;   /* in direct page -- DO NOT USE YET, see below */
```

⚠ **`direct` storage class is unverified.** No PoC in `src/c/` exercises it. An early attempt to use it in [main.c](../../src/c/main.c) (2026-04-25) was rolled back after a different bug (DCC `char`→`int` printf promotion) was wrongly blamed on `direct`. The mechanism is therefore *untested* rather than known-broken. A dedicated PoC must establish the right pattern (likely an explicit `#asm vsect ... endsect` reservation that does **not** clobber libc's own direct-page slots `_flacc`/`errno`/...) before production code uses `direct`. See [implementation/lessons-learned.md](../implementation/lessons-learned.md).

## Memory budget (estimates from GDD §23.3)

| Item | Estimate |
|------|----------|
| Map data (5×9 `Plot` struct) | ~360 B |
| Player data (4 × `Player`) | 64 B |
| Code | 20–30 KB (projected) |
| Stack | ≥2 KB |
| Graphics buffers | System-map (GrfDrv) — out-of-process |
| Runtime overhead (stdio, libc) | — |

Target: program + data < 64 KB.

## Static const data

Place terrain map, tile graphics, and other ROM-safe constants as `static const` so they land in the **code segment** rather than consuming data space:

```c
static const unsigned char terrain_map[MAP_ROWS][MAP_COLS] = { ... };
```

## Screen memory and the kernel block pool

**Correction 2026-05-01:** an earlier version of this page claimed CoVDG screens come from a "GrfDrv pool". That is wrong. Per [Tech Ref Ch. 4](../../docs/reference/NitrOS-9%20EOU%20Technical%20Reference.md) line 781: "Term_VDG or VERM uses VTIO/CoVDG while Term_win40/Term_win80 and all window descriptors use VTIO/(CoWin/CoGrf)/GrfDrv". **GrfDrv backs CoWin/CoGrf only.** CoVDG (`SS.AScrn`) does its own physical-RAM allocation through the kernel block layer (`F$AllRAM` / `D.BlkMap`).

Practically this still means screens compete with everything else for the same finite pool of physical 8 KB blocks — process DAT-image pages, `F$AllRAM` shared blocks, CoVDG screens, and CoWin/GrfDrv screens **all draw from the same `D.BlkMap`-tracked pool**. So a fatter parent process leaves less for child screens. `E$NoRAM (#207)` from `SS.AScrn` tells us the kernel block pool is exhausted, not anything CoVDG-specific.

**Observed limit:** allocating two 32K type-4 CoVDG screens fails (err #207). Two 16K type-2 CoVDG screens work when the parent process is small (e.g. `pocrnd`). With a heavier parent (`pioneer` linking score.c + input.c + sprintf path), even the 2×16K case can hit #207 — observed 2026-05-01.

To access screen bytes directly from the process:
1. `SS.ScInf` — get screen info
2. `F$MapBlk` — map 8 KB blocks of screen RAM into process space
3. Write / block-copy, then unmap or continue using mapped block

[poc_cwext.c](../sources/poc-sources.md) uses this pattern with a 32K back buffer copied 8K at a time.

## Cross-process shared memory

For the multi-process architecture (logic + render + sound), **`F$AllRAM` block IDs are bearer-style** — confirmed against the kernel assembly (`level2/modules/kernel/fallram.asm`):

- `F$AllRAM(B=count)` — allocates `count` contiguous physical 8 KB blocks. **Out:** `D` = starting block number. Marks the blocks "used" in the global `D.BlkMap`; no per-process owner is recorded.
- `F$MapBlk(B=count, X=block_num)` — maps blocks into the *caller's* DAT image, no ownership check. **Out:** `U` = mapped logical address.
- Any process that knows the block number can map the same physical bytes.
- `F$ClrBlk(B=count, U=address)` — **unmaps** from the calling process's DAT image. Does NOT free the physical block. (Confirmed against [`fclrblk.asm`](D:\retro\nitros9\level2\modules\kernel\fclrblk.asm) — only modifies `P$DATImg`.)
- `F$DelRAM(B=count, X=block_num)` — actually deallocates the physical block by clearing the `RAMinUse` bit in `D.BlkMap`. Confirmed against [`fdelram.asm`](D:\retro\nitros9\level2\modules\kernel\fdelram.asm).
- **Caveat:** physical blocks are not auto-freed on process exit. Whoever allocated the block must call `F$DelRAM` (typically after `F$ClrBlk`-ing their own mapping), or the block leaks until reboot.

For named, refcount-managed sharing, `F$VModul` registers a runtime-built module so children find it via `F$Link`. See [ipc.md](ipc.md) for the full discussion and recommended baseline.

## Related

- [stack.md](stack.md) — full platform overview
- [dcc.md](dcc.md) — compiler flags affecting layout
- [covdg.md](covdg.md), [cowin.md](cowin.md) — graphics memory consumers

## Sources

- `docs/design/MULE_GDD.md` §23.3
- `AGENTS.md` CoVDG memory findings
- `docs/reference/NitrOS-9 EOU Technical Reference.md` (F$MapBlk details — stub)
