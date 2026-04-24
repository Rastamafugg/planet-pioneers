# Turn Sequence

One full turn = one round. Rounds repeat until the game ends (6 rounds Beginner, 12 Standard/Tournament).

```
For each round:
  1. Summary Report (net worth display)
  2. Land Grant (one free plot per player, in turn order)
  3. [Standard/Tournament] Land Auction (0–6 plots, average 1)
  4. Colony Management (each player in turn order)
  5. Production (automatic, all plots simultaneously)
  6. [Sometimes] Pre-production Random Event
  7. Commodity Auctions (Smithore, Food, Energy, [Crystite])
  8. Post-auction wealth update
```

## Phase pages

- [Summary Report](scoring.md) — net worth display; rank update timing
- [Land Grant](#land-grant) — below
- [Land Auction](auctions.md) — Standard/Tournament
- [Colony Management](management-phase.md) — the iconic MULE walk
- [Production](production.md) — automatic resolution
- [Random Events](random-events.md) — pre-production, sometimes pre-auction
- [Commodity Auctions](auctions.md) — Smithore, Food, Energy, Crystite in order

## Development turn order — OPTION SPLIT

| Option | Source | Normal order | MULE-shortage reversal |
|--------|--------|--------------|-----------------------|
| A (GDD) | manual | **Poorest first** | N/A (always poorest first) |
| B (Planet M.U.L.E.) | planet-mule-rules | **Richest first** | Last-place first when corral is low |

**Strategic note:** Option B gives the leader first pick under normal conditions (a disadvantage to trailing players), then reverses when scarcity bites. Option A is always poorest-first.

**Turn-reversal threshold:**

| Option | Threshold |
|--------|-----------|
| A (GDD) | <7 MULEs in corral |
| B (Planet M.U.L.E.) | <8 MULEs in corral |

**Tie-breaker:** lower player index first.

## Land Grant (phase 2)

- One free plot per player per round.
- Cursor moves automatically left-to-right, top-to-bottom across the map. Joystick may steer to desired plot.
- Player presses button to claim the currently highlighted plot.
- Turn order: poorest player (lowest net worth) first. Tie → lower player index first.
- **Simultaneous-press window** (same plot, within ~100ms): poorer player wins; tied in wealth → lower index.
- Claimed plot flashes in winner's color.

## Sources

- `docs/design/MULE_GDD.md` §7, §9
