# Scoring and Victory

## Net worth formula

```
net_worth = cash + (sum over commodities of goods[i] × last_auction_price[i]) + (owned_plots × 500)
```

`LAND_VALUE = 500` per plot, regardless of purchase price (so an auctioned plot bought for $1200 still contributes $500 to net worth).

## Summary Report phase

Displayed after each round (and before the game begins):

- Each player's color, species, net worth
- Player ranking (1st–4th)
- Round number / total rounds

**Rank update timing:** rankings update **only after Summary phase** (Planet M.U.L.E. clarification). The leaderboard does **not** update mid-round.

**Duration:** ~3 seconds, or any button to advance.

## Victory

After the final round:

1. Compute each player's net worth.
2. **Standard/Tournament: colony survival check.** If combined net worth < **$60,000** (`COLONY_SURVIVAL_THRESHOLD`), **all players lose** regardless of relative ranks ("colony failed").
3. Otherwise, highest net worth wins.

Beginner mode has no colony survival threshold.

## Tiebreakers

Across the design: **lower player index first** for ties (net worth, Land Grant button-press timing, etc.).

## Related

- [turn-sequence.md](turn-sequence.md) — Summary is phase 1 of each round
- [economy.md](economy.md) — commodity prices used in valuation

## Sources

- `docs/design/MULE_GDD.md` §8, §17
