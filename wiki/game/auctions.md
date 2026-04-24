# Auctions

**Status:** STUB — detailed mechanics not yet ingested (GDD §10, §14).

## Two kinds of auction

### Land Auction (Standard/Tournament, after Land Grant)

- 0–6 plots auctioned each round (random, avg 1).
- Flashing black square indicates the plot; chosen by game, not players.
- All players press buttons to signal readiness, then each player's character moves **up** the screen to indicate their bid (higher position = higher bid).
- Dashed line shows current high bid. Timer counts down.
- When timer expires: highest bidder above minimum wins and pays their bid. No bids above minimum → plot unsold.
- Plot contributes $500 to net worth regardless of purchase price (see [scoring](scoring.md)).

### Commodity Auction (each round, phase 7)

Runs once per commodity in order: **Smithore, Food, Energy, [Crystite]**.

Each player declares buyer-or-seller side, then moves their character vertically to set their price. Buyers move **down** (lower asking), sellers move **up** (higher asking). When a buyer and seller cross, a trade happens at that price, in units limited by whoever has less capacity/stock.

**Collusion (Tournament only):** two players can share a price and avoid bidding against each other.

## Critical auction thresholds

| Commodity | Min |
|-----------|-----|
| Food | $15 |
| Energy | $10 |
| Smithore | $14 (Opt A) or $35 (Opt B) |
| Crystite | $50 (Opt A) or $48 (Opt B) |
| All max | $265 |

## Timer

`AUCTION_TIMER_TICKS = 3600` = 60 seconds at 60 ticks/sec.

## Ingest priority

This page needs substantive expansion from GDD §10 and §14 before the commodity-auction phase can be implemented. Open targets:
- Exact UI semantics (button-press → side declaration → price movement)
- Trade resolution algorithm when multiple buyers/sellers overlap
- Store price determination before and during auction
- Crystite auction is special: only Store buys

## Sources

- `docs/design/MULE_GDD.md` §10 (Land Auction) and §14 (Commodity Auctions) — NOT YET INGESTED IN DETAIL
