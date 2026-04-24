# Colony Management Phase

Each player, in [turn order](turn-sequence.md), walks their character around the town and map. Timed: out of time → turn ends immediately.

## Time allocation — function of Food

Time bar is proportional to Food supply.

- **Full food (enough for this round):** 45 seconds
- **Partial food:** proportional (3/4 food → 3/4 time)
- **No food:** ~4.5 seconds (enough to reach the Pub and end the turn for money)

**Formula:** `management_ticks = BASE_TICKS + (food_owned × FOOD_TICKS_PER_UNIT)` at 60 ticks/sec.

| Constant | Value |
|----------|-------|
| `BASE_TICKS` | 60 (normal species) |
| `MANAGEMENT_TICKS_HUMANOID` | 40 (expert species — penalty) |
| `FOOD_TICKS_PER_UNIT` | 10 |
| `MANAGEMENT_TICKS_NO_FOOD` | 27 (~4.5 sec, Planet M.U.L.E.) |

When the time bar hits 0 the turn ends regardless of state.

## Town layout

Store column (column 4). Player starts at center (store entrance).

| Building | Location | Function |
|----------|----------|----------|
| Corral | Bottom center | Purchase MULEs |
| Food Outfitting Shop | Left side | Outfit MULE for Food |
| Energy Outfitting Shop | Right side | Outfit MULE for Energy |
| Smithore Outfitting Shop | Upper left | Outfit MULE for Smithore |
| Crystite Outfitting Shop | Upper right | Outfit MULE for Crystite (Tournament) |
| Pub | Lower left | Voluntarily end turn for money (`PUB_RATE = $2`/tick remaining) |
| Land Office | Lower right | Mark owned plot for sale (Std/Tournament) |
| Assay Office | Upper center | Assay a plot for Crystite deposits (Tournament) |

## MULE workflow

1. **Get a MULE:** walk into Corral. MULE follows you out. Charged immediately at MULE price.
2. **Outfit:** walk (with MULE) into the desired outfitting shop. Outfit cost deducted.
3. **Install:** walk off left/right edge of town onto the map, navigate to an owned plot, stand on plot's house marker, press button.
   - **Beginner:** pressing anywhere outside the exact house position → MULE **runs away**.
   - **Standard/Tournament:** installing on the wrong commodity-type plot → MULE runs away.

## Corral

**Initial MULEs — OPTION SPLIT:** Option A (original manual) = 16; Option B (Planet M.U.L.E.) = 14.

**Corral capacity — OPTION SPLIT:** Option A (GDD) = 25 implementation-defined; Option B = 14.

When empty (Standard/Tournament), no MULEs available until the store builds more from Smithore (2 Smithore → 1 MULE, takes one full turn).

## Selling a MULE — OPTION SPLIT

| Option | Refund |
|--------|--------|
| A (GDD) | Purchase price only (outfitting cost lost) |
| B (Planet M.U.L.E.) | Purchase price + outfitting cost |

## Assay Office (Tournament)

Reveals Crystite deposit level on a plot. Cost: $25 (Option A) or free (Option B).

## Pub

Trade remaining ticks for cash: `$2 × remaining_ticks`. Also the default "end turn safely when out of food."

## Related

- [commodities.md](commodities.md) — outfit costs
- [economy.md](economy.md) — MULE purchase price formula
- [platform/timing.md](../platform/timing.md) — 60-tick/sec timing base

## Sources

- `docs/design/MULE_GDD.md` §11
