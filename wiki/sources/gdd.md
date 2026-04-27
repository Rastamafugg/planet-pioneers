# Source: M.U.L.E. Game Design Document

**Path:** `docs/design/MULE_GDD.md` (1654 lines, v1.0)
**Status:** Primary source of truth for game design (per AGENTS.md). Sections 1–7, 11, 18, 19–24 substantively ingested; 8–10 and 12–17 still stub-level in the wiki.

## What it is

A 24-section GDD targeting DCC C on NitrOS-9 EOU / CoCo 3. Synthesizes the original M.U.L.E. manual with the "Planet M.U.L.E." modern rules site. Throughout, it flags places where the two sources disagree and presents both as **Option A (original)** vs **Option B (Planet M.U.L.E.)**, leaving the implementation choice deliberately open.

## Section map

| § | Title | Wiki page |
|---|-------|-----------|
| 1 | Game Overview | [game/overview.md](../game/overview.md) |
| 2 | Difficulty Modes | [game/modes.md](../game/modes.md) |
| 3 | Players and Characters | [game/players.md](../game/players.md) |
| 4 | Planet Map | [game/map.md](../game/map.md) |
| 5 | Terrain and Production | [game/production.md](../game/production.md) |
| 6 | Commodities | [game/commodities.md](../game/commodities.md) |
| 7 | Turn Sequence | [game/turn-sequence.md](../game/turn-sequence.md) |
| 8 | Summary Report | (stub — included in turn-sequence) |
| 9 | Land Grant | (stub — [auctions.md](../game/auctions.md)) |
| 10 | Land Auction | (stub — [auctions.md](../game/auctions.md)) |
| 11 | Colony Management | [game/management-phase.md](../game/management-phase.md) |
| 12 | Production phase | (stub — [production.md](../game/production.md)) |
| 13 | Random Events | [game/random-events.md](../game/random-events.md) (stub) |
| 14 | Commodity Auctions | [game/auctions.md](../game/auctions.md) (stub) |
| 15 | Economic Model | [game/economy.md](../game/economy.md) (stub) |
| 16 | Tournament-Only Features | (stub) |
| 17 | Scoring and Victory | [game/scoring.md](../game/scoring.md) |
| 18 | Computer Player AI | [game/ai.md](../game/ai.md) |
| 19 | UI Screen Specifications | (not ingested) |
| 20 | Input System | [platform/input.md](../platform/input.md) |
| 21 | Audio | [platform/sound.md](../platform/sound.md) |
| 22 | Data Structures | [implementation/data-structures.md](../implementation/data-structures.md) |
| 23 | Platform Notes | [platform/stack.md](../platform/stack.md), [platform/memory.md](../platform/memory.md) |
| 24 | Constants Reference | (consolidated across game pages) |

## Key recurring themes

- **Option A vs Option B.** The GDD does not commit to a ruleset; every page in `game/` mirrors this ambiguity until the implementation makes a choice. See individual pages for specifics.
- **Combined net worth $60,000** is the shared colony survival threshold in Standard/Tournament (§2.2).
- **Direct page storage** (`direct` storage class in DCC) is recommended for `g_state` and hot-path globals (§22, §23.3).
- **Graphics** specified as CoWin type 8 (320×200×16); the PoCs have since explored CoVDG type 2 (160×192×16) as a lower-memory alternative. See [platform/covdg.md](../platform/covdg.md).
