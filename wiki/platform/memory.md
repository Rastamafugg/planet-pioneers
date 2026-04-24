# Memory Layout

## Process model

NitrOS-9 EOU Level 2 gives each process a **single 64 KB logical address space**, mapped from 512 KB physical RAM via an 8 KB-page MMU. Code + data + stack must fit within this 64 KB window (though the **GrfDrv**-managed graphics memory lives in the system map and does not count against process space).

## DCC layout

- **Code segment** — program code
- **Data segment (Y-relative)** — DCC places globals and statics here
- **Stack** — sized via `-m=` flag (e.g. `-m=4k`, `-m=8k`, `-m=32k`)
- **Direct page** — 255 bytes; access by `direct` storage class

## Direct page discipline

255 bytes total. Reserve for hot-path state. Per GDD §23.3:

- `GameState g_state` (~12 bytes) — mode, round, turn order, active player, auction commodity, crystite price
- Hot-path variables — whatever's in tight rendering/animation loops

```c
direct GameState g_state;   /* in direct page */
```

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

## GrfDrv and screen memory

Video screens allocated via [CoVDG](covdg.md) `SS.AScrn` or [CoWin](cowin.md) come from a **separate pool managed by GrfDrv** in the system memory map, not from process address space.

**Observed limit:** allocating two 32K type-4 CoVDG screens fails (err #207). Two 16K type-2 CoVDG screens work — this is what enables [poc_cvdg16](../sources/poc-sources.md)'s page-flipping.

To access screen bytes directly from the process:
1. `SS.ScInf` — get screen info
2. `F$MapBlk` — map 8 KB blocks of screen RAM into process space
3. Write / block-copy, then unmap or continue using mapped block

[poc_cwext.c](../sources/poc-sources.md) uses this pattern with a 32K back buffer copied 8K at a time.

## Related

- [stack.md](stack.md) — full platform overview
- [dcc.md](dcc.md) — compiler flags affecting layout
- [covdg.md](covdg.md), [cowin.md](cowin.md) — graphics memory consumers

## Sources

- `docs/design/MULE_GDD.md` §23.3
- `AGENTS.md` CoVDG memory findings
- `docs/reference/NitrOS-9 EOU Technical Reference.md` (F$MapBlk details — stub)
