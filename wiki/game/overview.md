# Game Overview

1–4 player turn-based economic strategy game. Players colonize the planet **Irata**, claim land, install **M**ulti-**U**se **L**abor **E**ntities (MULEs) on plots to produce commodities, and trade at auction. Highest net worth when the colony ship returns wins.

Tension: players must **cooperate enough to keep the colony alive** (combined-net-worth survival threshold in Standard/Tournament) while **competing for personal advantage**.

## Game length

| [Mode](modes.md) | Rounds |
|------|--------|
| Beginner | 6 |
| Standard | 12 |
| Tournament | 12 |

One round = one full [turn sequence](turn-sequence.md).

## Core loop

Each round, every player:
1. Receives a free plot in [Land Grant](turn-sequence.md), optionally bids on an auctioned plot.
2. Walks around the town in the [Colony Management phase](management-phase.md) — buys MULEs, outfits them for a commodity, installs them on owned plots.
3. Produces [commodities](commodities.md) automatically based on terrain × MULE type × bonuses.
4. Sometimes gets hit with a [random event](random-events.md).
5. Buys and sells in [commodity auctions](auctions.md).

## Win condition

After the final round, compare [net worth](scoring.md). In Standard/Tournament, **all players lose** if combined net worth is below $60,000 — "the colony failed."

## Sources

- `docs/design/MULE_GDD.md` §1, §2, §17
