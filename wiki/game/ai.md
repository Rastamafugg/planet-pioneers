# Computer Player AI

**Status:** STUB — details not yet ingested (GDD §18).

Computer players are mechanically identical to humans, with one cosmetic difference: they **do not visibly perform the colony management walk** — the system resolves their MULE placement decisions without animating the town/map walk.

## Target function signatures (GDD §22.11)

```c
void ai_land_grant(unsigned char player);
void ai_management(unsigned char player);
void ai_auction_declare(unsigned char player, unsigned char commodity);
void ai_auction_move(unsigned char player);
```

## Tournament bonus

Computer players receive **+$200 starting cash** (`AI_TOURNAMENT_BONUS = 200`) in Tournament mode to offset human skill advantage.

## Ingest priority

**Low until implementation begins.** The strategy guide (`docs/design/mule-strategy-guide.md`, [strategy-guide stub](../sources/strategy-guide.md)) is the main well for AI heuristics; GDD §18 describes baseline AI behavior targets. When expanding, cover:

- Land grant priority (which terrain/plot to pick given current holdings?)
- Management decisions (how many MULEs to buy, which commodity to outfit?)
- Auction behavior (declare buyer vs seller; where to set price?)
- Difficulty scaling — Beginner opponents play worse, Tournament better

## Sources

- `docs/design/MULE_GDD.md` §18, §22.11 (NOT YET INGESTED)
- `docs/design/mule-strategy-guide.md` (strategy reference for AI)
