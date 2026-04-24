# Platform Stack

Target environment for the build: **NitrOS-9 EOU on Tandy Color Computer 3 with 512K RAM, compiled with DCC C.**

## Layers

```
┌─────────────────────────────────┐
│  Planet Pioneers (C program)    │  — src/c/*.c, built with dcc
├─────────────────────────────────┤
│  DCC C Runtime (libc + _os9)    │  — K&R-style, 16-bit int
├─────────────────────────────────┤
│  NitrOS-9 EOU Level 2           │  — multitasking, paged memory
│    VTIO / CoWin / CoVDG / VRN   │  — drivers we touch
├─────────────────────────────────┤
│  MC6809E / HD6309 + GIME        │  — CPU + video/memory chip
└─────────────────────────────────┘
```

## CPU / hardware

- **CPU:** Motorola 6809E (CoCo 3) or Hitachi 6309 (compatible, faster native mode).
- **GIME** (Graphics Interrupt Memory Enhancer): custom Tandy chip handling video, paged MMU, interrupts.
- **RAM:** 512K (NitrOS-9 EOU target configuration).
- **Clock:** 60 Hz NTSC, so 60 ticks/sec throughout the system. See [timing.md](timing.md).

## Operating system: NitrOS-9 EOU

Multi-user, multi-tasking, paged OS. Processes get a single 64K logical address space (Level 2) with MMU-mapped 8K blocks from 512K physical memory. Graphics memory is managed by **GrfDrv** in the system map — does **not** consume process address space, which is crucial for 16K/32K graphics screens.

Key subsystems we use:
- **VTIO / CoVDG** — low-level video path, 160×192×16 or 320×192×16 screens. See [covdg.md](covdg.md).
- **CoWin** — windowing/graphics path on top of VTIO, with higher-level drawing commands and overlays. See [cowin.md](cowin.md).
- **VRN** (`/nil` device) — user-space IRQ signals for timing. See [timing.md](timing.md).
- **SCF/keyboard/joystick** — input via GetStat calls on terminal paths. See [input.md](input.md).
- **SS.Tone** — audio via VTIO path. See [sound.md](sound.md).

## Compiler: DCC

K&R-style C for 6809/6309. See [dcc.md](dcc.md) for constraints and flags. Recommended starter invocation:

```
dcc mule.c map.c players.c auction.c ai.c render.c sound.c -m=32k -o=mule
```

## Memory budget

See [memory.md](memory.md). Highlights:

| Item | Estimated size |
|------|---------------|
| Map data (5×9 Plot struct) | ~360 B |
| Player data (4 × 16 B) | 64 B |
| Code segment | ~20–30 KB (projected) |
| Stack | ≥2 KB |
| Graphics buffers | Managed by GrfDrv, system map — out-of-process |
| Direct page | 255 B available; reserved for `g_state` + hot-path vars |

Target: program + data < 64 KB (single process address space on Level 2).

## Reference documents

- `docs/reference/NitrOS-9 EOU Technical Reference.md` — system calls, drivers ([summary](../sources/nitros9-docs.md))
- `docs/reference/NitrOS-9 EOU Level 2 Windowing System Manual.md` — CoWin, screen types
- `docs/reference/NitrOS-9 EOU Operating System User's Guide.md` — end-user shell
- `docs/reference/DCC C Compiler System Users Guide.md` — compiler driver/flags ([summary](../sources/dcc-docs.md))
- `docs/reference/DCC C Compiler System Library Reference.md` — libc + `_os9()`

## Sources

- `docs/design/MULE_GDD.md` §23
- `AGENTS.md`
