---
type: source
tags: [platform, hardware, coco3, gime, reference]
updated: 2026-05-03
---

# Assembly Language Programming for the CoCo 3 (Tepolt, 1987)

Bare-metal hardware reference for the Tandy Color Computer 3, written as an addendum to Tepolt's original CoCo 1/2 book. **1659 lines** at `docs/reference/Assembly Language Programming for the CoCo3.md` (PDF also present).

## Scope

The book documents what changed between the CoCo 1/2 and the CoCo 3 — almost everything new is in the **ACVC** (a.k.a. **GIME** — Graphics Interrupt Memory Enhancer):

- Chapter 1 — system overview, block diagram, I/O connectors.
- Chapter 2 — **Palette registers** (`$FFB0–$FFBF`), composite vs RGB color encodings, alternate color set bit.
- Chapter 3 — **Physical & virtual memory**: 8K pages, executive vs task **PAR sets** (`$FFA0–$FFAF`), 19-bit physical addresses, MMU enable/select bits in `$FF90`/`$FF91`, ROM/RAM mode (`$FFDE`/`$FFDF`).
- Chapter 4 — **Hi-res displays**: ACVC control regs `$FF90–$FF9F`. 32/40/64/80-column text with per-character attribute byte (blink, underline, fg palette 8–15, bg palette 0–7). Graphics modes up to 640×225×16, three buffer-byte formats (A/B/C). Vertical & horizontal scroll registers.
- Chapter 5 — **Lo-res (legacy VDG) displays**: still reachable via `$FF22` and SAM mode toggles `$FFC0–$FFC5`, vertical offset `$FFC6–$FFD3`.
- Chapter 6 — **Interrupts**: ACVC IRQ/FIRQ enable in `$FF90`. `IRQEN $FF92` / `FIRQEN $FF93` select sources (Timer, Hbord, **Vbord** = vertical-sync 60 Hz, SerIn, Kybd/Joy, Cart). 12-bit Timer in `$FF94`/`$FF95`, decremented at 63.5 µs (or 70 ns if `Init1` bit 5 set). Vector table `$FFF2–$FFFF`, primary jump table at `$FEEE–$FEFF`, secondary jump table at `$0100–$010F`.
- Chapter 7 — Reset initialization (palette init values, executive PAR set 38–3F), advanced graphics tricks (color-blending, smooth horizontal scroll using N back-buffers), MPU speed switch (`$FFD8` low / `$FFD9` high — 0.89 / 1.78 MHz; serial & cassette I/O break in high), redundant dedicated addresses, `FF22` split (bits 7–3 → ACVC VDG, bits 2–0 → PIA 2 side B).
- Appendix A — hi-res text character set / video codes.
- Cross Reference — every dedicated address `$FF00–$FFFF` and what it controls.

## Why it matters for Planet Pioneers

We run under **NitrOS-9 EOU**, which **owns all of this hardware**. We do not poke `$FFAx`/`$FFBx`/`$FF9x` directly — `F$AllRAM`/`F$MapBlk` mediate physical RAM, GrfDrv mediates palette + video buffer, VRN mediates the 60 Hz heartbeat that this book calls **Vbord**. The book is therefore reference-grade context, not an instruction manual for our codebase.

It's still useful when:
- Reading NitrOS-9 driver source (CoVDG, CoWin, GrfDrv, Clock) — it explains what the registers they write actually do.
- Debugging memory / display behavior at the hardware level (e.g. why CoVDG screens compete for 8K blocks — see [memory.md](../platform/memory.md)).
- Reasoning about timing: the 60 Hz VRN heartbeat ([timing.md](../platform/timing.md)) is the OS-level abstraction over the GIME's `Vbord` IRQ described in Ch. 6.
- Understanding the executive/task PAR split that NitrOS-9 uses (kernel mapped via executive set; processes via task set — Ch. 3 + 7).

## Distilled into

- New page: [GIME hardware reference](../platform/gime.md) — palette regs, MMU/PARs, hi-res mode bits, ACVC interrupts, MPU speed.
- Cross-links from [memory.md](../platform/memory.md), [timing.md](../platform/timing.md), [stack.md](../platform/stack.md).

## Companion source

`docs/reference/Assembly Language Programming for the Color Computer.md` (16 439 lines) is the prerequisite CoCo 1/2 book Tepolt refers to as "The Book." Not yet ingested; tackle on demand if a question touches 6809 instruction-level detail, the original VDG, or PIA programming.
