# Timing and VSync

## Base clock

60 Hz NTSC CoCo 3 → **60 ticks/second** across all system timing.

| Duration | Ticks |
|----------|-------|
| 1 second | 60 |
| [Management phase](../game/management-phase.md) full-food | 2700 (45 s) |
| Auction timer | 3600 (60 s) |
| Management phase no-food | 27 |

## Available timing primitives

### `F$Sleep` — sleep for N ticks

Via `tsleep()` in DCC. Coarse but documented. The generic pacing call.

### `F$Time` — wall clock read

Returns **whole seconds only.** Insufficient for FPS measurement (AGENTS.md: "if a test reports unexpectedly slow timing, treat that as an observed fact and avoid assuming the cause without a discriminating test"). Useful for high-level RNG seeding and coarse intervals.

### VRN `/nil` `SS.FSet` — 1/60s user signal

Proven by [poc_vsync.c](../sources/poc-sources.md) (113 lines).

```c
#define SS_FSET  0xC7
/* Register for signal 0x7F every 1/60 sec on /nil */
```

Gives a **documented 1/60-second timing gate for measurement.** This is NOT direct GIME VBlank or HSync access — the AGENTS.md lesson explicitly cautions against describing it as "guaranteed tear-free synchronization." It is a reliable software-level 60Hz heartbeat, sufficient for frame pacing but not for hardware-tight video sync.

Used in [poc_gfx.c](../sources/poc-sources.md) as the VRN VIRQ timing gate driving sprite movement.

### VRN architecture (Tech Ref Ch.8)

`/nil` is an SCF-backed null device. All VRN VIRQ + RAM functionality runs through GetStat/SetStat on a path opened to `/nil`. Three primitives sit behind it:

| SetStat | Code | Behavior | Notes |
|---------|------|----------|-------|
| `SS.FSet` | $C7 | "FS2+" VIRQ — programmable interval, repeating or one-shot, with VIRQ + signal counters | What we use today |
| `SS.FClr` | $A8 | Clears FS2/FS2+ VIRQ for caller's path | |
| `SS.KSet` | $C8 | "KQ3-style" VIRQ — fixed signal `$80`, fixed 1/60s interval, no metrics | Lighter to service in IRQ but inflexible |
| `SS.KClr` | $C9 | Clears KQ3-style VIRQ | |

**Naming gotcha:** `SS.KSet` / `SS.KClr` are **NOT** keyboard-related despite the K prefix — the K stands for *King's Quest III*, the original consumer of the VIRQ pattern. Real keyboard control is `SS.KySns` (see [input.md](input.md)). Easy to misread when grepping the Tech Ref.

**System-wide cap: 4 simultaneous VIRQ entries**, keyed on (process ID, path number). A single process can hold multiple by opening multiple paths to `/nil`. Plenty of headroom for our logic+render+sound architecture.

VIRQ delivery rides the standard signal path, so all the [signal semantics in ipc.md](ipc.md#signal-semantics-tech-ref-ch2) apply — pending-signal cap of 1, intercept required, etc.

## Page flipping and vsync

- **[CoWin](cowin.md) does NOT provide true page flipping or guaranteed VBlank sync** — command-buffering only.
- **[CoVDG](covdg.md)** allows allocating multiple screens and switching with `SS.DScrn` — this is the viable page-flip path, constrained by memory to 16K screen type 2.

## Sound timing interaction

`SS.Tone` is **blocking** — the calling process pauses for the full tone duration. This means audio in the main process steals management-phase ticks. Production architecture must run a **dedicated sound process** that reads `{freq, dur, amp}` tuples from a pipe. See [sound.md](sound.md).

## Sources

- `AGENTS.md` timing findings
- `docs/design/MULE_GDD.md` §23.2
- `src/c/poc_vsync.c`, `src/c/poc_gfx.c`, `src/c/poc_sound.c`
