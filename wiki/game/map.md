# Planet Map

## Grid

**5 rows × 9 columns = 45 cells.** Column 5 (0-indexed: col 4) is the Town/Store column and not ownable. River runs vertically through the store column. **44 plots** are available for ownership.

```
Col:  1    2    3    4    [5]   6    7    8    9
Row 1: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
Row 2: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
Row 3: [  ] [  ] [  ] [  ] [STO] [  ] [  ] [  ] [  ]
Row 4: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
Row 5: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
```

Plots indexed `plot[row][col]`, row ∈ [0,4], col ∈ [0,8], excluding col 4.

**Map is fixed per game instance — same map each session.** Only Crystite deposits are randomized (Tournament).

## Terrain

Each plot has a terrain type:

| Terrain | Notes |
|---------|-------|
| **River** | Column 4 plots and specific plots left/right of center. Best for Food production. No Smithore or Crystite possible. |
| **Plains** | Open flat terrain. Best for Energy. |
| **Mountains (1–3 symbols)** | Symbol count (1, 2, or 3) determines Smithore bonus. Best for Smithore. 3-symbol mountains are premium. |

Canonical terrain from GDD §22.10 (subject to verification against original):

```
Row 0: PL PL MT PL  RV PL MT PL PL
Row 1: MT PL PL PL  RV PL PL PL MT
Row 2: PL PL MT PL  ST PL MT PL PL
Row 3: MT PL PL MT  RV MT PL PL MT
Row 4: PL MT PL PL  RV PL PL MT PL
```

Mountain symbol count per plot (GDD §22.10):

```
0 0 2 0  0 0 1 0 0
1 0 0 0  0 0 0 0 3
0 0 1 0  0 0 2 0 0
2 0 0 3  0 3 0 0 1
0 1 0 0  0 0 0 2 0
```

Store this as a `static const` array to conserve RAM (stored in code segment, not data segment).

## Plot state

Each plot tracks:
- `owner` (−1 = unowned, 0–3 = player)
- `mule_type` (−1 = none, or commodity index 0–3)
- `for_sale` flag (Standard/Tournament only — via [Land Office](management-phase.md))
- `crystite` deposit level (0–3; Tournament only)

See [implementation/data-structures.md](../implementation/data-structures.md) for the `Plot` struct.

## Related

- [production.md](production.md) — how terrain translates to production output
- [turn-sequence.md](turn-sequence.md) — Land Grant and Land Auction phases

## Sources

- `docs/design/MULE_GDD.md` §4, §22.10
