# Terrain and Production

Each [plot](map.md) with an installed MULE produces its commodity every turn. Two bonuses stack on top of base production.

## Base production

| Commodity | River | Plains | Mountain (per symbol) |
|-----------|-------|--------|----------------------|
| Food | 4 | 2 | 1 |
| Energy | 2 | 3 | 1 |
| Smithore | 0 | 1 | 1 + mountain_symbol_count |
| Crystite | N/A | deposit_level | deposit_level |

Mountains produce Smithore = `1 + mountain_symbols`, so a 3-symbol mountain mine = 4 base Smithore. River plots **cannot** produce Smithore or Crystite.

## Bonuses (stack)

**Adjacency / Economies of Scale (EOS):**
- If ≥2 of a player's plots producing the same commodity are orthogonally adjacent, **each adjacent plot** gets **+1** production.
- Flat +1 regardless of chain length.
- Must be owned by the **same player** — opponent adjacency doesn't count.

**Learning Curve (LCT):**
- If a player owns **≥3 total plots** producing the same commodity anywhere on the map, all those plots get **+1 production per full group of 3** (i.e. 6 plots → +2 each).
- Stacks with EOS.

Example: 3 Smithore mines, 2 adjacent. LCT adds +1 to each of the 3. EOS adds an additional +1 to the 2 adjacent ones. Total bonus: 5 units spread across the three plots.

## Energy requirement

- **Every non-Energy MULE needs 1 unit of Energy from the previous turn's stock to produce.**
- Energy MULEs self-fuel (no external energy cost).
- Insufficient Energy → those non-Energy MULEs produce **0**.

**Energy deficit allocation — OPTION SPLIT:**

| Option | Source | Behavior |
|--------|--------|----------|
| A (GDD) | manual | Implementation-defined; suggested prioritize by plot index |
| B (Planet M.U.L.E.) | planet-mule-rules | One random non-Energy plot produces 0 per unit short (shown as dark lightning bolt) |

## Variance by mode

| [Mode](modes.md) | Variance |
|------|----------|
| Beginner | None — exact base + bonuses |
| Standard | Random variance applied to base, then + bonuses |
| Tournament | Same as Standard |

**Variance formula — OPTION SPLIT:**

| Option | Source | Formula | Range | Shape |
|--------|--------|---------|-------|-------|
| A (GDD) | manual | `rand(0,base) + rand(0,base)` | [0, base×2] | Bell curve |
| B (Planet M.U.L.E.) | planet-mule-rules | `base + rand(-3, +3)` | [base−3, base+3] | Uniform ±3 |

Option A: low-base plots (base=1) zero out frequently. Option B: fixed spread; plots with base=0 can still produce up to 3.

## Crystite production (Tournament)

Crystite deposit levels are fixed at game init but **randomized per session**, unlike terrain (fixed across all sessions).

Base production = deposit level (1, 2, or 3) before bonuses. River plots = level 0.

**High-deposit count — OPTION SPLIT (3-way):**

| Option | Source | High plots | Spread |
|--------|--------|-----------|--------|
| A (GDD v1.0) | GDD | 2 | Orthogonal adjacency: Med→Low |
| B (original manual) | manual | 3 | Orthogonal |
| C (Planet M.U.L.E.) | planet-mule-rules | 4 | Circular/radial bloom |

## Related

- [commodities.md](commodities.md) — what each commodity is used for
- [economy.md](economy.md) — how production feeds prices
- [random-events.md](random-events.md) — events modify production outcomes

## Sources

- `docs/design/MULE_GDD.md` §5
