# CoWin Graphics

CoWin is the higher-level windowing graphics system, mandated by NitrOS-9 EOU. Uses documented drawing commands; originally the GDD's default choice.

## Opening a window

```c
g_win_path = open("/w1", 3);      /* update mode */
/* send DWSet escape sequence to configure window as graphics type N */
```

`DWSet` configures the path as a graphics window with a chosen screen type. Typical types:

| Type | Resolution | Colors | Used in |
|------|-----------|--------|---------|
| 6 | 320×200 | 4 | [poc_gfx.c](../sources/poc-sources.md) |
| 8 | 320×200 | 16 | [poc_cwext.c](../sources/poc-sources.md), [poc_tiles.c](../sources/poc-sources.md); GDD §23.1 default |

## Drawing commands

All via escape sequences or `I$Write`:

- `SetDPtr` — set draw pointer
- `Line`, `Bar` — primitive shapes
- `FColor`, `BColor` — set foreground/background color
- `Palette` — assign color values
- `GetBlk`, `PutBlk` — grab/blit a rectangular block (sprites)
- `OWSet` — create overlay text windows on top of a graphics window

## GET/PUT findings (from AGENTS.md, verified by [poc_gfx.c](../sources/poc-sources.md))

- `GetBlk`/`PutBlk` are **fast enough for small sprite tests** when the buffer contains the **union of restored background plus the new sprite**.
- **One 40×40 union buffer per movement direction reduced flicker better than separate background restore + sprite put.**
- Use **byte-aligned X coordinates** where possible; 8-pixel step tests succeeded.

## External back buffer pattern ([poc_cwext.c](../sources/poc-sources.md))

When a single visible screen isn't enough:

1. Create CoWin type 8 screen (32K, 320×200×16).
2. `SS.ScInf` + `F$MapBlk` to map visible screen RAM into process space.
3. `SS.ARAM` to allocate a **32K external back buffer**.
4. Render to back buffer, then copy to visible screen **one 8K block at a time** (`BLK_COUNT = 4`).

This trades memory and copy cost for a clean back-buffer model — no true page flip, but avoids mid-frame rendering artifacts.

## Critical limitation

**CoWin command buffering does NOT provide true page flipping or guaranteed VBlank synchronization.** For page flipping, use [CoVDG](covdg.md) instead. For frame pacing, use the VRN `/nil` 1/60s gate described in [timing.md](timing.md).

## Text & overlays

CoVDG has no text primitives. For HUDs, score panels, prompts, and the auction ticker, see [cowin-text.md](cowin-text.md) — covers DWSet/OWSet/OWEnd, the Ch.5 text command bytes, font caveats, and the `Palette`/`DefColr` escape sequences (the documented alternative to the deferred `SS.PalSet` codepath).

## Related

- [cowin-text.md](cowin-text.md) — overlay text windows + text commands + palette
- [covdg.md](covdg.md) — lower-level alternative with page flip support
- [memory.md](memory.md) — GrfDrv + process mapping
- [timing.md](timing.md) — pacing to 60 Hz
- [input.md](input.md) — joystick/keyboard on the same or related paths

## Sources

- `AGENTS.md` CoWin GET/PUT findings
- `docs/reference/NitrOS-9 EOU Level 2 Windowing System Manual.md` (stub)
- `docs/design/MULE_GDD.md` §23.1
- `src/c/poc_gfx.c`, `src/c/poc_cwext.c`, `src/c/poc_tiles.c`
