# CoWin Text & Overlay Windows

How Planet Pioneers will draw text (HUDs, score panels, prompts, auction tickers) on top of the CoVDG graphics screens owned by the render child. CoVDG itself has no text primitives, so any character output must go through a CoWin path or overlay window.

Ingested from `docs/reference/NitrOS-9 EOU Level 2 Windowing System Manual.md` Ch.1, Ch.3 (DWSet, OWSet, OWEnd, Palette, DefColr), Ch.5 (text commands) on 2026-04-26.

## Window kinds

- **Device window** (`/w1`–`/w15`) — top-level. Allocated via `iniz /wN` then sized/configured by `DWSet`. **Cannot overlap** other device windows. Each runs on its own screen unless `STY=0` (current screen) or `STY=$FF` (currently displayed screen).
- **Overlay window** — opened on top of a device window via `OWSet`. Inherits the device window's screen type. Stacked logically (only top is writable). All overlays must reside fully inside the parent device window.

For a graphics screen plus a status bar, the standard pattern is: one type-8 device window covering the play area, then an `OWSet` text overlay for the bar.

## DWSet (`ESC $20`)

Parameters: `STY CPX CPY SZX SZY PRN1 PRN2 PRN3` (foreground, background, border palette register numbers).

Screen types:

| STY | Size      | Colors | RAM   | Type     |
|-----|-----------|--------|-------|----------|
| $FF | currently displayed screen | — | — | — |
| $00 | process's current screen | — | — | — |
| 1   | 40×25     | 8/8    | 2000  | text     |
| 2   | 80×25     | 8/8    | 4000  | text     |
| 5   | 640×200   | 2      | 16000 | graphics |
| 6   | 320×200   | 4      | 16000 | graphics |
| 7   | 640×200   | 4      | 32000 | graphics |
| 8   | 320×200   | 16     | 32000 | graphics |

Notes:
- All windows on a screen must share the screen's type.
- DWSet auto-clears to background.
- A path silently swallows output until a DWSet (explicit or descriptor default) lands.
- **GIME bug:** modes 5–8 only show 199 of the 200 lines until you scroll up.

## OWSet (`ESC $22`)

Parameters: `SVS CPX CPY SZX SZY PRN1 PRN2`.

- `SVS=$01` — save-and-restore the underlying area at `OWEnd`. Costs RAM.
- `SVS=$00` — no save; underlying pixels are destroyed. Cheaper; redraw on close.
- Sizes are in **standard characters of the underlying device window's resolution**. Use `SS.ScSiz` GetStat at startup if the device window doesn't cover the whole screen.
- Stack is LIFO — close (`OWEnd`, `ESC $23`) overlays in reverse order to write to a lower one.

For the auction ticker / status bar pattern: pick `SVS=0` if redraw is cheap, `SVS=1` if the underlying tiles are expensive to recompute.

**Resolved 2026-04-30 ([poc_owtxt](../implementation/poc-catalog.md), [poc_owtxt2](../implementation/poc-catalog.md)):** OWSet on the CoVDG VDG path is silently swallowed — produces nothing on screen. A parallel CoWin device window via `iniz /wN` + DWSet `STY=$FF` (currently displayed screen) is *also* silently swallowed: `/w1`, `/w2`, `/w7`, `/w8` all open cleanly but no window lands on the CoVDG-allocated screen. **There is no escape-driven CoWin text path onto a CoVDG-allocated screen.** Phase 7a HUD/score therefore renders **bitmap glyphs directly via the render child's putpx** — see [poc-catalog.md](../implementation/poc-catalog.md) (render.c entry) `R_OP_TEXT`.

## Palette (`ESC $31 PRN CTN`) and DefColr (`ESC $30`)

- Color byte CTN is 6-bit RGB: `00rRgGbB` (low + high bit per channel) → 64 distinct colors.
  - White `$3F`, Black `$00`, Std Blue `$09`, Std Green `$12`, Std Red `$24`.
- Changing a register repaints **every pixel using that register, system-wide on that screen**, including the border if it's pointed at the same register.
- Palette registers are **per-screen, not per-window**; FG/BG register *selection* is per-window.
- `DefColr` resets to monitor-defaults (RGB or composite, picked by `montype`). Default values can be globally overridden via `SS.DFPal` SetStat.
- **Resolved 2026-04-30 ([poc_owtxt2](../implementation/poc-catalog.md) P2):** `ESC $31 PRN CTN` written **directly to the CoVDG VDG path** (the same path that accepts `ESC $21` Select) successfully repaints palette registers system-wide on the displayed screen. No CoWin window or DWSet is needed. Tech Ref shows there is no SetStat function code for setting an individual palette register — only `SS.Palet` ($91) GET and `SS.DFPal` ($97) SET-default exist — so this escape is the only documented per-register set, and it works through the path the render child already owns. **`R_OP_PALETTE` resolution:** the deferred Phase 4 op becomes "write 4-byte `ESC $31 PRN CTN` to `g_path`" in the render child — no `SS.PalSet`, no `SS.DFPal`.

## Text commands (Ch. 5)

Work on text **and** graphics windows. All single-byte unless noted.

| Code        | Effect |
|-------------|--------|
| `01`        | Home cursor. |
| `02 X Y`    | Move cursor; coords sent as `X+$20`, `Y+$20`. |
| `03`        | Erase current line. |
| `04`        | Erase from cursor to end of line. |
| `05 20`     | Cursor off. |
| `05 21`     | Cursor on. |
| `06`/`08`   | Cursor right / left. |
| `09`/`0A`   | Cursor up / down. |
| `0B`        | Erase to end of screen. |
| `0C`        | Clear screen + home. |
| `0D`        | Carriage return. |
| `1F 20/21`  | Reverse video on/off. |
| `1F 22/23`  | Underline on/off. |
| `1F 24/25`  | Blink on/off (text screens only). |
| `1F 30/31`  | Insert / delete line at cursor. |

After that, normal `I$Write` of ASCII bytes prints at the cursor.

### Fonts on graphics screens

- The system standard font is auto-assigned at DWSet **if it's already in memory**. If not, every character renders as `.` until a `Font` command after `GPLoad`.
- 8-pixel-wide fonts render **several × faster** than 6-pixel or proportional fonts (`PropSw`).
- A pre-bolded font is faster than a normal font with `BoldSw` on.
- For a single-color HUD, a screen type 5 (2-color, 16 KB) device or overlay window renders text fastest of all — fonts are natively 1-bit.

## Performance tips relevant to HUDs

From the Ch.4/5 optimization notes:

- Window scrolling, `GetBlk`/`PutBlk`, horizontal lines, boxes, bars, and flood fills are fastest on **8-byte boundaries** and in **multiples of 8 bytes** wide. On a 320×200×16 (type-8) screen that's every 16 pixels.
- **Full-width windows scroll faster** than partial-width windows of the same height.
- Horizontal/vertical lines are much faster than diagonal lines.

Implication: align HUD overlay X/width to 16-pixel boundaries on type-8 screens.

## Resolved 2026-04-30 (Phase 6.5, [poc_owtxt](../implementation/poc-catalog.md) + [poc_owtxt2](../implementation/poc-catalog.md))

1. **OWSet on the CoVDG path: no.** The escape `ESC $22 …` is silently swallowed when written to the VDG path that owns a CoVDG-allocated screen.
2. **Parallel CoWin device window on the same screen: no.** `iniz /wN` (tested `/w1`, `/w2`, `/w7`, `/w8`) + DWSet `STY=$FF` opens the path cleanly but produces no visible window over the CoVDG-displayed screen. CoWin's window mechanism does not see CoVDG-allocated screens as targets it can attach to.
3. **`Palette` (`ESC $31 PRN CTN`) escape on the VDG path: yes.** Repaints palette registers system-wide on the displayed screen. This is the route for `R_OP_PALETTE`.
4. **Font preload: moot.** With (1) and (2) negative, no CoWin text path lands on the screen, so the system font's preload state is irrelevant. **HUD/score uses render-child-owned bitmap glyphs**, demonstrated by [poc_owtxt2](../implementation/poc-catalog.md) P3.

The architectural consequence for Phase 7a: HUD/score/prompts are not text-driver output. They are **bitmap glyphs compiled into the render child** and drawn via the existing `putpx` primitive, behind a new `R_OP_TEXT(x, y, str, color)` command in [poc-catalog.md](../implementation/poc-catalog.md) (render.c entry).

## Related

- [cowin.md](cowin.md) — graphics drawing primitives (Bar, Line, GetBlk/PutBlk).
- [covdg.md](covdg.md) — the screen-allocation path actually owned by the render child.
- [memory.md](memory.md) — process map / GrfDrv.

## Sources

- `docs/reference/NitrOS-9 EOU Level 2 Windowing System Manual.md`
  - Ch.1 "Device Windows" / "Overlay Windows" L101–187
  - Ch.3 DWSet L506–555, OWSet L807–831, OWEnd L797–805, Palette L833–869, DefColr L305–335
  - Ch.4 optimization notes L1103–1105
  - Ch.5 text commands + optimization tips L1339–1384
