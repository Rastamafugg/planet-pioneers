---
type: concept
tags: [platform, hardware, coco3, gime]
updated: 2026-05-03
---

# GIME hardware reference

The **ACVC** (Advanced Color Video Chip), commonly called **GIME**, is the custom Tandy LSI that handles video, the MMU, and most new interrupt sources on the CoCo 3. NitrOS-9 EOU owns this hardware; Planet Pioneers code never writes these registers directly. This page is a map of what the OS is doing under the hood, distilled from [Tepolt's CoCo 3 book](../sources/coco3-asm-tepolt.md).

## Dedicated address ranges (top of page 7)

| Range | Function |
|-------|----------|
| `$FF00–$FF03` | PIA 1 (keyboard, joystick, hsync, peripheral) |
| `$FF20–$FF23` | PIA 2 (DAC, sound, cassette, vsync). `$FF22` bits 7–3 split off to drive the legacy VDG inside the ACVC |
| `$FF40–$FF5F` | Cartridge SCS area (gated by `Init0` bit 2) |
| `$FF90–$FF9F` | **ACVC control registers** (Init0, Init1, IRQEN, FIRQEN, Timer, video mode/resolution/border, vertical & horizontal offsets, vertical scroll) |
| `$FFA0–$FFAF` | **MMU PARs** — executive set `$FFA0–$FFA7`, task set `$FFA8–$FFAF` |
| `$FFB0–$FFBF` | **Palette registers** 0–15 |
| `$FFC0–$FFD3` | SAM mode bits (V0–V2) and SAM vertical offset (F0–F6) — pair-addressed (set/clear) |
| `$FFD8` / `$FFD9` | MPU speed: writing `$FFD8` selects 0.89 MHz, `$FFD9` selects 1.78 MHz |
| `$FFDE` / `$FFDF` | SAM TY: ROM/RAM mode (`$FFDE`) vs all-RAM mode (`$FFDF`) |
| `$FFF2–$FFFF` | 6809 interrupt vector table (ROM-resident; primary jump table in RAM at `$FEEE–$FEFF`, secondary at `$0100–$010F`) |

## MMU (Chapter 3)

- **Virtual extent:** 8 pages × 8 KB = 64 KB (the MPU's address space).
- **Physical extent:** 64 pages × 8 KB = 512 KB.
- **Two PAR sets** — executive (`$FFA0–$FFA7`) and task (`$FFA8–$FFAF`). `Init1` bit 0 selects which set the CPU uses.
- A PAR holds the upper 6 bits of the 19-bit physical address; the lower 13 bits come straight from the virtual address.
- `Init0` bit 6 enables/disables the MMU. With it disabled, the 64 KB virtual extent maps to the **upper 64 KB of physical memory**.
- Virtual `$FF00–$FFFF` is **never** translated — it always reaches the dedicated addresses. So virtual page 7 has only 8 KB − 256 bytes of usable RAM.
- Physical pages **`$3C–$3F`** can be flipped between RAM and three ROM configurations via `Init0` bits 0–1 (Color BASIC / Extended / Super Extended / cartridge).

NitrOS-9 EOU uses the executive set for the kernel and the task set for processes; this is exactly the controller-vs-task pattern Tepolt describes in §3.

## Palette registers (Chapter 2)

Sixteen 6-bit registers at `$FFB0–$FFBF`. Each holds a code 0–63 chosen from the **composite color set** or the **RGB color set**:

- **Composite encoding** — bits 5–4 = brightness/saturation pair, bits 3–0 = hue angle (0 = gray, 1 = green, … 11 = blue, 15 = bluish green). Code `$3F` is hardwired to white.
- **RGB encoding** — `R1 G1 B1 R0 G0 B0`. Each primary picks one of four intensities (none / low / medium / high).
- Writing **`$FF98` bit 5** invokes the **alternate color set**, shifting non-gray hues 180° around the wheel (used to fake legacy artifact-color behavior).

Hi-res text uses **palette regs 0–7 for background, 8–15 for foreground**, selected by the per-character attribute byte's BKC2-0 / FRC2-0 fields.

## Hi-res displays (Chapter 4)

Selected when `Init0` bit 7 is clear.

- **Buffer base** lives in `$FF9D` (Vert Offset 1) + `$FF9E` (Vert Offset 0) — the upper 16 bits of a 19-bit physical address. Buffer must start on an 8-byte boundary.
- **Video mode `$FF98`** — `BP` (text vs graphics), `H50` (50 Hz PAL), `LPR2-0` (pixel/character row height).
- **Video resolution `$FF99`** — `VRES1-0` (192/200/225 graphic rows = 24/25/28 text rows), `HRES2-0` × `CRES1-0` together select the 22 hi-res graphics modes.
- **Hi-res text** — two bytes per cell: video code (0–7F, ASCII-compatible above 32) + attribute byte (Blink, Underline, FRC2-0, BKC2-0).
- **Hi-res graphics** — three buffer formats:
  - **A** (`CRES = 00`): 1 bit/pixel, 2 colors from palettes 0–1.
  - **B** (`CRES = 01`): 2 bits/pixel, 4 colors from palettes 0–3.
  - **C** (`CRES = 10`): 4 bits/pixel, 16 colors from palettes 0–15.
  - `CRES = 11` blanks the screen — useful for invisible buffer updates.
- **Vertical smooth scroll** uses `$FF9C` SC2-0 (1/8-line increments); when it wraps, advance the buffer base by one row width.
- **Horizontal scroll** via `$FF9F` X6-0 (2-byte = 4/8/16-pixel coarse increments). `HE` bit forces a 256-byte-wide buffer for panoramic scrolling.

## Lo-res / legacy VDG displays (Chapter 5)

Selected when `Init0` bit 7 is set. The VDG functions (text + SG4 + Gx graphics modes) are now inside the ACVC but still controlled via `$FF22` bits 7–3 plus the SAM mode/offset registers at `$FFC0–$FFD3` (each bit set or cleared by writing to a separate dedicated address). Same buffer-format catalog as the original CoCo 1/2.

## Interrupts (Chapter 6)

- **`Init0` `$FF90`** — bit 5 enables ACVC IRQ generation, bit 4 enables ACVC FIRQ generation.
- **`IRQEN $FF92`** / **`FIRQEN $FF93`** — six source bits each: Timer, Hbord, Vbord, SerIn, Kybd/Joy, Cart. Reading the register both identifies the source and acknowledges the pending interrupt to the ACVC.
- **Timer** — 12-bit reload value in `$FF94`/`$FF95`. Decremented every 63.5 µs (or 70 ns if `Init1` bit 5 is set). Drives both blink rate and the Timer IRQ; auto-reloads on hitting zero.
- **Vbord = 60 Hz vertical-sync IRQ** — this is the GIME signal that NitrOS-9's VRN service translates into the `/nil` `SS.FSet` heartbeat we use for frame pacing. See [timing.md](timing.md).
- **Vector table at `$FFF2–$FFFF`** points into ROM, which long-branches into the **primary jump table** at `$FEEE–$FEFF` (RAM, modifiable). The primary table in turn long-branches into the **secondary jump table** at `$0100–$010F` (legacy CoCo 1/2 layout).
- `Init0` bit 3 (`MC3`) makes virtual `$FE00–$FEFF` always reach physical RAM page `$3F` regardless of PAR 7 — that's how the OS keeps the primary jump table reachable while remapping.

## Reset state (Chapter 7)

After reset the ACVC clears itself: MMU disabled, ROM/RAM mode selected, ROM mapping = 16 K internal + 16 K external, all interrupts off. The reset initializer then loads palette regs, sets PARs (executive 38–3F covering the upper 64 K), and copies ROM into RAM before jumping to BASIC. Relevant for understanding the system state NitrOS-9's bootstrap inherits, not for our application code.

## What we never touch

Application code in Planet Pioneers does **not** write any of `$FF90–$FFFF`. All access is mediated:

| Hardware concern | OS-level path we use |
|------------------|----------------------|
| Allocate 8 KB blocks of RAM | `F$AllRAM` / `F$MapBlk` ([memory.md](memory.md)) |
| Allocate / display / free a video buffer | `SS.AScrn` / `SS.DScrn` / `SS.FScrn` on a CoVDG path ([covdg.md](covdg.md)) |
| Set palette / draw / scroll | CoWin / CoVDG draw commands ([cowin.md](cowin.md)) |
| 60 Hz frame heartbeat (Vbord IRQ) | VRN `SS.FSet` on `/nil` ([timing.md](timing.md)) |
| Keyboard / joystick read | `SS.KySns` / `SS.Joy` ([input.md](input.md)) |
| Sound output | `SS.Tone` ([sound.md](sound.md)) |
| Switch to 1.78 MHz | NitrOS-9 EOU runs the 6309 in native mode; we don't toggle `$FFD8`/`$FFD9` ourselves |

## Sources

- [coco3-asm-tepolt.md](../sources/coco3-asm-tepolt.md) — full source summary
- `docs/reference/Assembly Language Programming for the CoCo3.md` Ch. 1–7 + Cross Reference
