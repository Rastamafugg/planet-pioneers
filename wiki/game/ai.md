# Computer Player AI

GDD §18 baseline behavior for the 3 CPU opponents that play alongside the single human in every game.

Computer players are mechanically identical to humans, with one cosmetic difference: they **do not visibly perform the colony management walk** — the system resolves their MULE placement decisions without animating the town/map walk.

## Tournament bonus

Computer players receive **+$200 starting cash** (`AI_TOURNAMENT_BONUS = 200`) in Tournament mode to offset human skill advantage (GDD §3 / §16).

## Turn priority (GDD §18.1)

CPU players evaluate possible actions in this fixed priority order. The first applicable rule wins; lower rules act as tiebreakers / diversification once higher needs are met.

1. **Food/Energy self-sufficiency** — ensure ≥1 Food MULE and ≥1 Energy MULE before diversifying.
2. **Smithore acquisition** — in Standard/Tournament, pursue Smithore for MULE-manufacturing leverage.
3. **Crystite discovery** (Tournament only) — assay promising plots; acquire high-deposit plots.
4. **Adjacency optimization** — prefer plots adjacent to same-commodity plots already owned (feeds the EOS bonus, see [production.md](production.md)).
5. **Learning curve** — aim for 3+ same-commodity plots to trigger the LCT bonus.

## Land grant (GDD §18.2) — feeds [phase 7b](../implementation/roadmap.md)

Per-plot expected value:

```
EV(plot) = terrain_production_value(plot) * remaining_rounds + 500
```

CPU selects the **highest-EV unowned plot**. The `+500` baseline keeps any unowned plot more attractive than passing.

**Tie-break (CPU vs CPU on same plot):** lower player index wins. (GDD does not spell out CPU-vs-human ties at land grant; deferred until 7b implementation.)

## Auction policy (GDD §18.3) — feeds [phase 7e/7f](../implementation/roadmap.md)

| State | Stance |
|-------|--------|
| Has surplus of commodity | Declare **Seller** |
| Has shortage of commodity | Declare **Buyer** |

Price-walk under the auction timer:

- **As Seller:** start at the maximum price; only move down as the timer runs low.
- **As Buyer:** start at the minimum price; only move up as the timer runs low.
- **Response rate** is intentionally slightly slower than a human to give humans a fair chance.

Concrete reaction-rate / step-size constants are not in the GDD — to be tuned in 7f against the [commodity-auctions](auctions.md) PoC and recorded in [lessons-learned](../implementation/lessons-learned.md).

## MULE management (GDD §18.4) — feeds [phase 7c/7d](../implementation/roadmap.md)

- **Always purchase** a MULE if affordable AND has ≥1 unworked plot.
- **Outfit priority:**
  - Food first if food supply is low.
  - Energy first if energy supply is low.
  - Otherwise commodity matching plot's terrain advantage (Smithore on mountains, etc.).
- **Installation is instant** for CPU — no visible walk sequence (or briefly animated; choice deferred to 7c).

"Low" thresholds are not specified in the GDD; tuning happens during 7d once production numbers are running.

## Function signatures (GDD §22.11)

The GDD pre-declares these handles. They will be implemented inside the relevant 7x phase, not as a standalone phase 6 module:

```c
void ai_land_grant(unsigned char player);     /* 7b */
void ai_management(unsigned char player);     /* 7c/7d */
void ai_auction_declare(unsigned char player, unsigned char commodity);  /* 7e/7f */
void ai_auction_move(unsigned char player);   /* 7f */
```

## Decision: phase 6 dissolved into 7a–7g (2026-04-26)

**Original roadmap:** phase 6 was a standalone "minimal AI module" landing before any playable phase, refined per-phase in 7.

**Revised:** phase 6 is dissolved. Each AI policy lands inside the phase that consumes it (land-grant AI in 7b, management AI in 7c/7d, auction AI in 7e/7f). Phase 7a (Summary/Scoring) needs no AI.

**Why:**
- Pre-building a module before its call sites exist forces guesses about `GameState` shape and phase entry hooks. Per the project rule against speculative abstractions, co-locating policy with consumer is preferred.
- GDD §18 is small enough (one formula + one stance table + one priority list) that each policy fits comfortably alongside its phase's human-side code.
- 7a doesn't need AI at all, so phase 6 was not actually blocking the first playable phase.

**Risk accepted:** if a later 7x phase needs to call into another phase's AI (e.g. auction-AI consulting management-AI for cash reserves), a small shared `ai.c` module gets factored out at that point. Refactoring 1–2 stubs into a module is cheaper than designing the module up front.

## Sources

- `docs/design/MULE_GDD.md` §18 (lines 976–1006), §22.11 function signatures, §3 / §16 tournament bonus
- `docs/design/mule-strategy-guide.md` (deeper heuristics — to be consulted during 7c–7f tuning)
