# Source: PoC source files

**Path:** `src/c/poc_*.c`
**Status:** Headers scanned; full bodies not yet line-by-line ingested.

See [implementation/poc-catalog.md](../implementation/poc-catalog.md) for what each PoC proves and the current state of findings.

| File | Lines | Target | Purpose |
|------|-------|--------|---------|
| `poc_cvdg.c` | 249 | CoVDG type 4 (320×192×16) | Page-flip attempt via `SS.AScrn`/`SS.DScrn`/`SS.FScrn`. Result: second 32K allocation fails (err #207). |
| `poc_cvdg16.c` | 609 | CoVDG type 2 (160×192×16, 16K) | Page-flip PoC with 9×5 map, masked/packed sprites, 2-frame directional walk cycles, staggered sprite paths. **Most advanced PoC.** |
| `poc_cwext.c` | 415 | CoWin type 8 (320×200×16) | External 32K back buffer via `SS.ARAM`, block-copy to visible screen 8K at a time. |
| `poc_gfx.c` | 409 | CoWin type 6 (320×200×4) | `GetBlk`/`PutBlk` sprite movement with union buffer per direction; VRN VIRQ timing gate. |
| `poc_sound.c` | 212 | /term | `SS.Tone` melody/amplitude/sweep/warble tests. **Confirms `SS.Tone` is blocking** — production needs dedicated sound process. |
| `poc_tiles.c` | 371 | CoWin type 8 | 9×5 tile grid + animated marker sprites rendered only via CoWin drawing commands (no `F$MapBlk`). |
| `poc_vsync.c` | 113 | /nil | `SS.FSet` VIRQ on `/nil` gives 1/60s user signal; measures 120-tick timing accuracy. |

Recent commit history (most recent first) shows focus on `poc_cvdg16.c`:

```
1004fa3 Stagger CoVDG sprite paths across doubled frame budget
957a598 Refactor poc_cvdg16 to 2-frame directional sprites
26154f9 Add packed walk cycles to poc_cvdg16
aa567ac Pack masked sprites in poc_cvdg16
7e1ed40 Optimize poc_cvdg16 cached sprite background restore
```

`poc_cvdg16` is the active frontier — animated sprites on a CoVDG page-flipped 16K screen with a 9×5 game-map tile grid.
