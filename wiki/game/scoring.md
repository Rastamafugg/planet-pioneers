# Scoring and Victory

Per-player net worth, leaderboard rules, and the three end-of-game screens (Summary, Colony Failure, Victory).

## Net worth formula

```
net_worth = cash
          + food_units     * food_last_price
          + energy_units   * energy_last_price
          + smithore_units * smithore_last_price
          + crystite_units * crystite_last_price   /* Tournament only */
          + owned_plot_count * 500
```

`LAND_VALUE = 500` per plot, regardless of purchase price (an auctioned plot bought for $1200 still contributes $500).

**Pre-auction valuation:** if no auction has occurred yet for a commodity, use its **starting price** — see [economy.md](economy.md). Phase 7a runs against round 1's pre-auction state, so all four `last_price` fields fall back to starting prices.

## Summary Report phase (GDD §8, §19.3.5)

Displayed **before each round** (including before round 1) and at the end after the last round produces.

**Content per player row:**
- Color swatch
- Species name
- Net worth ($)
- Land count

**Header:** `Round X of Y`.

**Ranking:** rows ordered by net worth descending; ties broken by **lower player index first** (consistent with all other tie rules in the design). Rank labels 1st–4th printed at row left.

**Rank update timing:** rankings update **only after the Summary phase** (Planet M.U.L.E. clarification). The leaderboard does **not** update mid-round.

**Duration:** ~3 seconds OR any button to advance. For 7a the gate is the existing `inp_pres(K_SPACE)` path (CTRL+SPACE skips to end).

## Victory screen (GDD §17.2, §17.4, §19.3.14)

After the final round's Summary:

1. Player with highest net worth wins; titled **"First Founder"**.
2. **Tie:** all tied players are declared winners (GDD §17.2). Rendering order falls back to lower player index first.
3. All four player net worths listed.
4. "Press button to exit" or auto-exit to OS.

(Consolation titles for 2nd–4th are mentioned in §17.4 but not enumerated; treat as "TBD copy" for 7a — render the rank number only.)

## Colony Failure screen (Standard/Tournament, GDD §17.3, §19.3.13)

After **each round** in Standard/Tournament, sum all player net worths:

- If `combined_net_worth < 60000` (`COLONY_SURVIVAL_THRESHOLD`): all players lose immediately; game ends.
- Display: **"COLONY FAILED"** banner, combined net worth shown vs. $60,000 target, all players shown as losers.

**Beginner mode has no survival check** (§2.1) — the colony-failure path never fires.

## Tiebreakers

Across the design: **lower player index first** for ties — net worth ranking, Land Grant button-press timing, AI evaluation ties.

## Game length

- Beginner: **6 rounds**
- Standard: **12 rounds**
- Tournament: **12 rounds**

(Phase 1 skeleton hardcodes `max_rounds = 6` regardless of mode — this is an existing-code defect to fix during 7a or flag for follow-up.)

## Related

- [turn-sequence.md](turn-sequence.md) — Summary is phase 1 of each round
- [economy.md](economy.md) — commodity prices (starting + last-auction) used in valuation
- [modes.md](modes.md) — survival threshold applies in Standard/Tournament only
- [implementation/data-structures.md](../implementation/data-structures.md) — `Player.net_worth`, `Player.plot_count`, `Player.goods[]`

## Sources

- `docs/design/MULE_GDD.md` §8 (Summary phase), §17 (Scoring/Victory), §19.3.5/13/14 (screens), §2 (game length), §19.2 (palette)
