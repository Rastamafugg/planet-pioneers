# Random Events

**Status:** STUB — details not yet ingested (GDD §13).

## Two types

1. **Player events** — target a single player. Triggered probabilistically each turn (`PLAYER_EVENT_CHANCE = 25%` in Std/Tournament).
2. **Planetary events** — affect all players / the colony globally.

## Known event types (with per-game occurrence caps)

| Event | Max per game | Effect (summary) |
|-------|-------------|-----------------|
| Pirate | 2 | Steals Smithore (Std) or Crystite (Tournament) |
| Pest | 3 | Damages Food production |
| Quake | 3 | Damages plots / MULEs |
| Sunspot | 3 | Energy disruption |
| Meteor | 2 | Plot damage |
| Radiation | 2 | Player health / production penalty |
| Fire | 2 | Plot / colony damage |
| Acid rain | 3 | Food damage |
| Wampus | — | Special creature; capture reward $50–$150 (Wampus bell sound event) |

## Data structure

```c
typedef struct {
    unsigned char type;     /* 0=player, 1=planetary */
    unsigned char subtype;  /* event index within type */
    signed char   target;   /* player index, or -1 for all */
    int           magnitude;/* variable effect size */
} RandomEvent;
```

## Ingest priority

**High** — blocks implementation of the random-event phase. Open targets from GDD §13:
- Exact effect magnitudes per event
- Timing within turn (pre-production per player vs pre-auction global)
- Beginner mode's "simplified random events" — which subset / what effects
- Wampus specifics (triggering, catching, reward calculation)

## Sources

- `docs/design/MULE_GDD.md` §13 (primary, NOT YET INGESTED)
- `docs/design/mule-manual.md` (original rules)
- `docs/design/Planet M.U.L.E. Rules _ Planet M.U.L.E..htm` (modern clarifications)
