# CoVDG Graphics

CoVDG is the lower-level video descriptor driver. It exposes allocatable **high-resolution graphics screens** controlled via three `I$SetStt` calls:

- `SS.AScrn` (`0x8B`) — **A**llocate a graphics screen of a given type
- `SS.DScrn` (`0x8C`) — **D**isplay an allocated screen (make it visible)
- `SS.FScrn` (`0x8D`) — **F**ree a previously allocated screen

These are called on a **VTIO/CoVDG VDG path**, **not** a CoWin window. Per AGENTS.md testing, `/verm` accepts `SS.AScrn` where `term_vdg` and `TERM_VDG` did not.

## Screen types tested

| Type | Resolution | Bits | Memory | Status |
|------|-----------|------|--------|--------|
| 2 | 160×192 | 16 colors | **16 KB** | Page-flip viable → used by [poc_cvdg16.c](../sources/poc-sources.md) |
| 4 | 320×192 | 16 colors | **32 KB** | Single-screen works; **second 32K allocation fails with error #207** |
| (6) | 320×200 | 4 colors | ~16 KB | Used via CoWin instead ([poc_gfx.c](../sources/poc-sources.md)) |
| (8) | 320×200 | 16 colors | ~32 KB | Used via CoWin ([poc_cwext.c](../sources/poc-sources.md), [poc_tiles.c](../sources/poc-sources.md)) |

**Takeaway:** for page flipping, **type 2 (160×192×16, 16K)** is the target. Type 4 (32K) works single-buffered but you can't allocate a back buffer of the same size — so no true page flip at 320×192.

## Page-flip sequence (PoC)

1. Open the VDG path (e.g. `/verm`, or a freshly-created CoVDG path).
2. `SS.AScrn` first screen (becomes screen 0).
3. `SS.AScrn` second screen (back buffer).
4. To switch visible screen: `SS.DScrn` the chosen screen.
5. **`SS.DScrn` only displays if the CoVDG path is the current interactive device.** Before expecting visible output, send the CoVDG **Select** escape `ESC $21` to the opened VDG path.
6. Before exit, **display screen 0** before freeing allocated high-res screens.

## Pitfalls (from AGENTS.md lessons)

- Use VDG paths, not CoWin windows, for these calls.
- Activate the CoVDG path via `Select` (`ESC $21`) before `SS.DScrn` output is visible.
- Don't free screens while they are displayed — switch back to screen 0 first.
- 32K × 2 allocation fails — plan memory around 16K screens when page-flipping.

## Addressing the screen buffer

The allocated screen's bytes live in GrfDrv's system-map space (not process address space). To draw pixels directly you need `SS.ScInf` to learn the mapping and `F$MapBlk` to map the 8K blocks into your process space. This pattern is used by [poc_cwext.c](../sources/poc-sources.md) for CoWin; the CoVDG PoCs primarily use documented draw calls rather than direct pixel writes.

## Current frontier: [poc_cvdg16.c](../sources/poc-sources.md)

Active development PoC. See commit log for recent work:
- Packed / masked sprites
- 2-frame directional walk cycles
- Staggered sprite paths (doubled frame budget)
- 9×5 tile map rendered on a 160×192 CoVDG type-2 screen with page flipping

## Related

- [cowin.md](cowin.md) — higher-level windowing alternative
- [memory.md](memory.md) — where screens live in memory
- [timing.md](timing.md) — 60Hz frame pacing

## Sources

- `AGENTS.md` CoVDG lessons
- `docs/reference/NitrOS-9 EOU Technical Reference.md` (not yet ingested section refs)
- `src/c/poc_cvdg.c`, `src/c/poc_cvdg16.c`
