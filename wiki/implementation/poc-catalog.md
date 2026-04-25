# PoC Catalog

Seven PoC files in `src/c/`. Each proves one platform capability before real game code is written. See [sources/poc-sources.md](../sources/poc-sources.md) for the raw index.

## poc_vsync.c — VRN timing gate (113 lines)

**Proves:** VRN `/nil` `SS.FSet` (0xC7) delivers signal `0x7F` every 1/60 second to a user process. Waits 120 ticks, reports measured wall-clock duration via `F$Time`.

**Finding:** 1/60s gate works as a **measurement-grade** timing heartbeat (AGENTS.md), not a hardware-tight vsync. See [platform/timing.md](../platform/timing.md).

## poc_gfx.c — CoWin sprite GET/PUT (409 lines)

**Target:** CoWin type 6 (320×200, 4 colors). Compile `-m=8k`.

**Proves:**
- `DWSet` creates a graphics window
- `Palette`, `FColor`, `SetDPtr`, `Bar` draw background/sprite
- `GetBlk` captures **one 40×40 movement buffer per direction** (the union of background + sprite shape)
- VRN `/nil` VIRQ provides 1/60s pacing
- `PutBlk` moves and restores the sprite in one command per step

**Finding (AGENTS.md):** 40×40 union-buffer-per-direction approach beats separate background-restore + sprite-put. Byte-aligned X (8-pixel steps) works well. See [platform/cowin.md](../platform/cowin.md).

## poc_cwext.c — CoWin + external back buffer (415 lines)

**Target:** CoWin type 8 (320×200×16, 32 KB). Compile `-m=8k`.

**Proves:**
- `SS.ScInf` + `F$MapBlk` to map the visible 32K screen
- `SS.ARAM` to allocate a separate 32K back buffer
- Render the back buffer, then **copy to visible screen 8 KB at a time** (4 blocks)

**Finding:** Viable pattern for CoWin double-buffering when a single visible screen isn't enough. No true page-flip — command buffering only. See [platform/cowin.md](../platform/cowin.md).

## poc_cvdg.c — CoVDG type 4 attempt (249 lines)

**Target:** CoVDG type 4 (320×192×16, **32 KB**). Compile `-m=4k`.

**Proves:** `SS.AScrn`/`SS.DScrn`/`SS.FScrn` work on a VTIO/CoVDG path.

**Finding:** Single-screen allocation OK; **second 32K allocation fails with error #207**. Page flipping at 320×192 not feasible. Pivoted to type 2 in `poc_cvdg16.c`. See [platform/covdg.md](../platform/covdg.md).

## poc_cvdg16.c — CoVDG type 2 page-flip + sprites (609 lines) ⭐ active

**Target:** CoVDG type 2 (160×192×16, 16 KB). Compile `-m=4k`.

Constants from header:
```
MAP_COLS=9, MAP_ROWS=5, TILE_W=17, TILE_H=38
SPR_W=8, SPR_H=8, WALK_FR=2
PATHFRAMES=150, NFRAMES=300
```

**Proves:** Two-screen CoVDG page-flipping + 9×5 tile map + animated sprites fits in 16 KB screen type 2.

**Recent commits** (current frontier):
```
1004fa3 Stagger CoVDG sprite paths across doubled frame budget
957a598 Refactor poc_cvdg16 to 2-frame directional sprites
26154f9 Add packed walk cycles to poc_cvdg16
aa567ac Pack masked sprites in poc_cvdg16
7e1ed40 Optimize poc_cvdg16 cached sprite background restore
```

This is the **most advanced PoC** and the direct precursor of the game's main render loop. The 9×5 tile map matches the game's [5-row × 9-column planet grid](../game/map.md).

## poc_sound.c — SS.Tone (212 lines)

**Target:** `/term`. Compile (no `-m`).

**Proves:** `SS.Tone` (I$SetStt 0x98), frequency range 0–4095 (high-range has widest variation), melody/amplitude/sweep/warble sequences.

**Finding (from PoC header):** `SS.Tone` is **BLOCKING** — the process pauses for the full duration. **Production architecture requires a dedicated sound process** reading `{freq, dur, amp}` tuples from a pipe. See [platform/sound.md](../platform/sound.md).

## poc_tiles.c — CoWin tile grid + markers (371 lines)

**Target:** CoWin type 8 (320×200×16). Compile `-m=24k`.

**Proves:** Pure-CoWin tile rendering — 9×5 map grid, two animated marker sprites, all via `DWSet`/`Palette`/`FColor`/`SetDPtr`/`Bar`. **Intentionally avoids** `F$MapBlk` and direct screen RAM.

**Finding:** High-level CoWin drawing path is viable for static-ish tile grids with a few moving markers. Constants: `TILE_W=35, TILE_H=40, MAP_OX=2, MAP_OY=0`.

## Progression summary

```
poc_vsync    →  1/60s timing baseline
   ↓
poc_gfx      →  CoWin sprite with union-buffer movement
poc_tiles    →  CoWin tile grid (no direct RAM)
poc_cwext    →  CoWin external back buffer
   ↓
poc_cvdg     →  CoVDG type 4 — blocked by memory
   ↓
poc_cvdg16   ⭐ CoVDG type 2 page-flip + tiles + sprites (current frontier)

poc_sound    →  SS.Tone confirmation; needs sound-process architecture
```

Next PoCs (not yet written), in order:

1. **`poc_ipc`** — port `SLPICPT.c` from [stocks-and-bonds](../sources/stocks-and-bonds.md); fork a child and round-trip a signal in C. Direct port; low risk.
2. **`poc_shmem`** — cross-process shared memory for the render path. Decides whether render runs in a child process (3-process design) or stays in main (2-process). See [platform/ipc.md](../platform/ipc.md) for transport candidates.
3. **Sound child process** — non-blocking `SS.Tone` driven by signals; payload via shared state. Architecturally de-risked once 1 + 2 land.

See [roadmap.md](roadmap.md) for the full phase plan.

## Sources

- `src/c/poc_*.c` headers
- `AGENTS.md` lessons
- Recent git log
