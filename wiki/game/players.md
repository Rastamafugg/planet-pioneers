# Players and Characters

## Player count

1–4 players. Human count selected at game start; computer fills remaining slots. Computer [AI](ai.md) is mechanically indistinguishable except it never visibly performs the colony management walk (goes straight to result).

## Color selection

Game start cycles through 4 colors. Each human player presses their joystick button to claim the currently displayed color. Colors map to player indices 0–3 (Red, Blue, Green, Yellow — exact palette values TBD, see [platform/covdg.md](../platform/covdg.md)).

## Species and economic tiers

8 species across 3 tiers:

| Tier | Species | Starting money | Management time modifier |
|------|---------|---------------|--------------------------|
| Expert | Humanoid | $600 | Reduced (`MANAGEMENT_TICKS_HUMANOID = 40`) |
| Normal | Mechtron, Gollumer, Bonzoid, Leggite, Packer, Spheroid | $1000 | Standard (`BASE_TICKS = 60`) |
| Beginner | Flapper | $1200 | Extended (bonus time) |

All players additionally start with Food and Energy (see below). Multiple players may pick the same species.

## Starting goods — OPTION SPLIT

| Option | Source | Food | Energy | Notes |
|--------|--------|------|--------|-------|
| A (original) | M.U.L.E. manual | ~1–2 units | ~1–2 units | $300 total value split at initial prices; remainder as cash |
| B (Planet M.U.L.E.) | [planet-mule-rules](../sources/planet-mule-rules.md) | 4 units | 2 units | Explicit unit counts, no cash conversion |

Option B is deterministic; Option A requires knowing initial commodity prices to resolve unit counts. **No implementation choice yet committed.**

## Character-selection UI

- 8 portraits displayed
- Joystick moves selector; button confirms
- Selected character appears enlarged in center

## Sources

- `docs/design/MULE_GDD.md` §3
