# Commodities

Four commodities indexed 0–3: Food, Energy, Smithore, Crystite. The fourth is Tournament-only.

```c
#define FOOD        0
#define ENERGY      1
#define SMITHORE    2
#define CRYSTITE    3
#define NUM_COMMODITIES 4
```

## Food (index 0)

- **Use:** determines [management-phase](management-phase.md) time — more Food = more time to walk around during the colony management phase.
- **Spoilage:** hold at most `needed + 2` units; excess discarded after production.
- **Need:** 1 unit per installed MULE (all types). Minimum need 1.
- **Price range:** $15–$250.
- **Outfit cost:** $25.

## Energy (index 1)

- **Use:** every non-Energy MULE consumes 1 unit/turn to produce. See [production §Energy](production.md).
- **Spoilage:** `needed + 1` cap.
- **Need:** count of player's non-Energy MULEs.
- **Price range:** $10–$250.
- **Outfit cost:** $50.

## Smithore (index 2)

- **Use:** store manufactures MULEs from it. **2 Smithore → 1 MULE** (takes one full turn). Standard/Tournament only.
- **Spoilage:** none (but see storage cap option below).
- **Outfit cost:** $75.

**Price range — OPTION SPLIT:**

| Option | Source | Beginner | Standard/Tournament |
|--------|--------|----------|---------------------|
| A (GDD) | manual | Fixed $50 | $25–$250 |
| B (Planet M.U.L.E.) | planet-mule-rules | Fixed $50 | $35–$260 |

## Crystite (index 3) — Tournament only

- **Use:** shipped off-planet; **no colony use**. Only the Store buys Crystite (no player-to-player).
- **Spoilage:** none (but see storage cap).
- **Outfit cost:** $100.

**Price range — OPTION SPLIT:**

| Option | Source | Range | Mechanism |
|--------|--------|-------|-----------|
| A (GDD) | manual | $50–$150 | Random per round (not supply/demand) |
| B (Planet M.U.L.E.) | planet-mule-rules | $48–$148 | Random per round |

## Storage caps — OPTION SPLIT

| | Food | Energy | Smithore | Crystite |
|--|------|--------|----------|----------|
| Spoil above | need+2 | need+1 | — | — |
| Option A cap | — | — | unlimited | unlimited |
| Option B cap | — | — | 50 units/player/round | 50 units/player/round |

## MULE outfitting costs summary

| Commodity | Outfit |
|-----------|--------|
| Food | $25 |
| Energy | $50 |
| Smithore | $75 |
| Crystite | $100 |

MULE **purchase price** is separate — see [economy.md](economy.md).

## Sources

- `docs/design/MULE_GDD.md` §6
