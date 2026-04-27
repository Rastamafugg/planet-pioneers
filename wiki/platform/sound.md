# Sound

## API: `SS.Tone`

`I$SetStt` code `0x98` on a terminal (VTIO) path.

Parameters:
- **Frequency:** relative counter **0–4095** (NOT Hz). Higher values → higher pitch. NitrOS-9 reference notes the **widest tone variation occurs at the high range**, so [poc_sound.c](../sources/poc-sources.md) intentionally uses high values.
- **Duration:** ticks (60/sec).
- **Amplitude:** 0–63.

Example (from GDD §23.5):

```c
void play_sound(unsigned int freq, unsigned char dur_ticks, unsigned char amp)
{
    struct registers r;
    r.rg_a = g_term_path;
    r.rg_b = 0x98;                      /* SS.Tone */
    r.rg_x = ((unsigned int)dur_ticks) | (((unsigned int)amp) << 8);
    r.rg_y = freq & 0x0FFF;
    _os9(0x8E, &r);                     /* I$SetStt */
}
```

## CRITICAL: `SS.Tone` is BLOCKING

The calling process pauses for the full tone duration. Proven by [poc_sound.c](../sources/poc-sources.md). Implication:

> **Production architecture requires a dedicated sound process** that reads `{freq, dur, amp}` tuples from a pipe.

A single blocking call during the management phase would freeze player movement. The sound subprocess decouples audio timing from gameplay process.

This pattern is not yet implemented — [poc_sound.c](../sources/poc-sources.md) only confirms the tone API and verifies melody / amplitude-accented sequences / sweeps / warbles work.

### Tech Ref Ch.9 confirmations (2026-04-26 ingest)

- `SS.Tone` blocks the caller until the tone completes, but **interrupts are NOT masked** during playback. That is why our sound-child design works: the parent can `F$Send` a wakeup/quit signal mid-tone and the child's `F$Icpt` handler runs immediately. Recorded as a positive confirmation, not a new finding.
- **Bit 15 of frequency = "8-bit volume" flag** for TC-9 hardware (lets MSB of X use full 0–255 amplitude). Irrelevant for our CoCo 3 target (use 0–63), but worth noting if anyone copies the call shape from a TC-9 source.
- **`SS.CDSig` / `SS.CDRel` are NOT a non-blocking SS.Tone mechanism.** They are serial-port-only — they signal on Carrier Detect / DSR change, handled by `sc6551`/`sc6850`/`s16550` drivers. (Initial Tier-1 ingest hypothesis was wrong — leaving this note so future readers don't re-litigate.) The current poll-based sound child remains the right design.

## Sound events (GDD §21.2)

| Event | Frequency | Duration | Notes |
|-------|-----------|---------|-------|
| MULE purchase | 400 | 5 | Brief confirmation |
| MULE installed | 500 | 10 | Ascending tone |
| Production unit | 300+n | 3 | n = unit number, rising pitch |
| MULE escaped | Descending | 15 | Falling tone |
| Wampus bell | 800 | 8 | Bell-like |
| Wampus caught | Ascending melody | 30 | |
| Auction trade | 600 | 5 | Per unit traded |
| Random event (good) | Ascending | 20 | |
| Random event (bad) | Descending | 20 | |
| Time warning | 400 | 5 | Repeated when <10 ticks remain |
| Game over | Descending long | 60 | |
| Victory | Fanfare | 90 | Multi-tone |

## Sources

- `docs/design/MULE_GDD.md` §21, §23.5
- `src/c/poc_sound.c` header comment
- `AGENTS.md` (indirectly — implied by sound process pattern recommendation)
