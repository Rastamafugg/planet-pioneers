# Economic Model

**Status:** STUB — detailed mechanics not yet ingested (GDD §15).

## Price determination

Commodity prices in the store respond to aggregate supply (player + store stock) and demand (colony needs + undeveloped plots). Drives the Commodity [Auctions](auctions.md).

`update_store_prices()` runs between turns; exact formula TBD (GDD §15).

## MULE purchase price — OPTION SPLIT

| Option | Source | Formula | Notes |
|--------|--------|---------|-------|
| A (GDD) | manual | `BASE + ((MAX_MULES − corral_count) × SCARCITY_FACTOR) + (undeveloped_plots × DEMAND_FACTOR)` | BASE=100, SCARCITY=50, DEMAND=25; cap $2000 |
| B (Planet M.U.L.E.) | planet-mule-rules | `2 × current Smithore price` | Direct supply link, simpler |

Option B creates a natural price coupling: Smithore scarcity → expensive MULEs. Option A gives independent tuning knobs.

In **Beginner** mode MULE price is fixed at $100.

## Colony survival

Standard/Tournament: combined net worth at game end must reach $60,000 (`COLONY_SURVIVAL_THRESHOLD`) or **all players lose**. See [scoring.md](scoring.md).

## Initial state — OPTION SPLIT (partial)

Per [planet-mule-rules](../sources/planet-mule-rules.md) and GDD §15.2:

| Item | Option A (original) | Option B (Planet M.U.L.E.) |
|------|---------------------|----------------------------|
| Initial MULEs in corral | 16 | 14 |
| Initial store Food | 16 | 8 |
| Initial store Energy | 16 | 8 |
| Initial store Smithore | 0 | 8 |

## Ingest priority

Medium — expand when working on store-price updates or auction integration. Open targets:
- Full price curve parameters per commodity (how steep is supply/demand response?)
- Store buy-sell spread rules
- Crystite: random per round regardless of stock (both options agree)

## Sources

- `docs/design/MULE_GDD.md` §15 (NOT YET INGESTED), §6, §11
