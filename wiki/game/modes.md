# Difficulty Modes

Three modes layered: Beginner ⊂ Standard ⊂ Tournament. The implementation must support all three.

## Beginner (6 rounds)

- Smithore price fixed at $50
- No land auction phase
- Cannot sell commodities below critical level
- No production variance (exact base + bonuses)
- MULE price fixed
- No Crystite
- No Collusion mechanic
- Simplified random events

## Standard (12 rounds)

- MULE price variable (depends on corral stock + undeveloped plots) — see [economy.md](economy.md)
- [Land auction](auctions.md) after each Land Grant
- Can sell below critical level (strategic risk)
- [Production variance](production.md) applied
- Store can run out of MULEs; rebuilt from Smithore 2:1
- **Colony survival threshold: $60,000 combined net worth** — all players lose if below
- MULEs run away if not installed on the correct plot square
- No Crystite, no Collusion

## Tournament (12 rounds)

Standard rules, plus:

- **Crystite** added as fourth commodity (Tournament-only, no colony use — shipped off-planet)
- **Collusion** mechanic enabled in auctions
- Computer players receive **+$200 starting bonus** to offset human skill
- Pirate event targets **Crystite** instead of Smithore
- Starting resources minimal: exactly enough to survive one turn

## Sources

- `docs/design/MULE_GDD.md` §2
