# M.U.L.E. — Game Design Document
## Target Platform: NitrOS-9 EOU, Tandy Color Computer 3, DCC C Compiler

**Version:** 1.0  
**Source Material:** M.U.L.E. manual, M.U.L.E. strategy guide  
**Implementation Language:** C (DCC compiler, 6809/6309 target)

---

## Table of Contents

1. [Game Overview](#1-game-overview)
2. [Difficulty Modes](#2-difficulty-modes)
3. [Players and Characters](#3-players-and-characters)
4. [Planet Map](#4-planet-map)
5. [Terrain and Production](#5-terrain-and-production)
6. [Commodities](#6-commodities)
7. [Turn Sequence](#7-turn-sequence)
8. [Phase: Summary Report](#8-phase-summary-report)
9. [Phase: Land Grant](#9-phase-land-grant)
10. [Phase: Land Auction (Standard/Tournament)](#10-phase-land-auction-standardtournament)
11. [Phase: Colony Management](#11-phase-colony-management)
12. [Phase: Production](#12-phase-production)
13. [Phase: Random Events](#13-phase-random-events)
14. [Phase: Commodity Auctions](#14-phase-commodity-auctions)
15. [Economic Model](#15-economic-model)
16. [Tournament-Only Features](#16-tournament-only-features)
17. [Scoring and Victory](#17-scoring-and-victory)
18. [Computer Player AI](#18-computer-player-ai)
19. [UI Screen Specifications](#19-ui-screen-specifications)
20. [Input System](#20-input-system)
21. [Audio](#21-audio)
22. [Data Structures (DCC C)](#22-data-structures-dcc-c)
23. [Platform Notes: NitrOS-9 EOU / CoCo 3](#23-platform-notes-nitros-9-eou--coco-3)
24. [Constants Reference](#24-constants-reference)

---

## 1. Game Overview

M.U.L.E. is a 1–4 player turn-based economic strategy game. Players colonize the planet Irata, claiming land, installing Multi-Use Labour Entities (MULEs) to produce commodities, and trading those commodities at auction. The player with the highest net worth when the colony ship returns wins.

The game models supply and demand, economies of scale, and the learning curve. Players must cooperate enough to keep the colony alive while competing for personal advantage.

**Game length:**
- Beginner: 6 rounds
- Standard: 12 rounds
- Tournament: 12 rounds

---

## 2. Difficulty Modes

### 2.1 Beginner

- 6 rounds total
- Smithore price fixed at $50
- No land auction phase
- Players cannot sell commodities below their critical level
- No production variance (base production is exact)
- MULE price fixed
- No Crystite commodity
- No Collusion mechanic
- Simplified random events

### 2.2 Standard

- 12 rounds total
- MULE price variable (depends on corral stock and undeveloped plots)
- Land auction phase after each Land Grant
- Players can sell below critical level (strategic risk)
- Production has random variance (0 to 2× base, see Section 12)
- Store can run out of MULEs; new MULEs built from Smithore (2:1)
- Colony must reach a combined net worth of $60,000 or all players lose
- MULEs run away if not installed on the correct plot square
- No Crystite, no Collusion

### 2.3 Tournament

- All Standard rules apply
- Crystite added as fourth commodity
- Collusion mechanic enabled in auctions
- Computer players receive +$200 starting bonus to offset human skill advantage
- Pirate event targets Crystite instead of Smithore
- Starting resources minimal: exactly enough to survive one turn

---

## 3. Players and Characters

### 3.1 Player Count

- 1–4 players
- Human players select their character; computer fills remaining slots
- Computer players are indistinguishable mechanically except they never visibly perform the colony management walk

### 3.2 Color Selection

At game start, the game cycles through 4 colors. Each human player presses their joystick button to claim the currently displayed color. Colors map to player indices 0–3.

Colors: Red, Blue, Green, Yellow (exact palette values: see Section 23)

### 3.3 Character Selection

After color selection, each player selects a character from 8 species. Only 3 economic tiers exist:

| Tier | Species | Starting Money | Management Time Modifier |
|------|---------|---------------|-------------------------|
| Expert | Humanoid | $600 | Reduced |
| Normal | Mechtron, Gollumer, Bonzoid, Leggite, Packer, Spheroid | $1000 | Standard |
| Beginner | Flapper | $1200 | Extended |

All players start with Food and Energy in addition to their cash.

**Starting goods — two options (select one per implementation):**

| Option | Source | Food | Energy | Notes |
|--------|--------|------|--------|-------|
| A (original) | M.U.L.E. manual | ~1–2 units | ~1–2 units | $300 total value split at initial prices; remainder as cash |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | 4 units | 2 units | Explicit unit counts; no cash conversion |

Option A requires knowing initial commodity prices to resolve unit counts. Option B is deterministic.

### 3.4 Character Selection UI

- 8 character portraits displayed
- Joystick moves selector; button confirms
- Selected character appears enlarged in center of screen

### 3.5 Duplicate Characters

Multiple players may select the same species. No restriction.

---

## 4. Planet Map

### 4.1 Grid Layout

The planet is a **5-row × 9-column** grid = 45 total cells.

- Column 5 (center) is the **Town/Store column** and is not ownable land. It contains the store building in the center row (row 3).
- The river runs vertically through column 5, represented visually by a winding path.
- **44 plots** are available for player ownership (all cells except the 5 store-column cells).

```
Col:  1    2    3    4    [5]   6    7    8    9
Row 1: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
Row 2: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
Row 3: [  ] [  ] [  ] [  ] [STO] [  ] [  ] [  ] [  ]
Row 4: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
Row 5: [  ] [  ] [  ] [  ] [riv] [  ] [  ] [  ] [  ]
```

Plots are indexed as `plot[row][col]` where row ∈ [0,4] and col ∈ [0,8], excluding col 4 (store column).

### 4.2 Terrain Types

Each plot has a terrain type assigned at game init (fixed per game instance, not randomized per session — same map each game).

| Terrain | Symbol Count | Notes |
|---------|-------------|-------|
| River | — | Column 4 plots (adjacent to store column) and specific left/right-of-center plots |
| Plains/Flatland | — | Open flat terrain |
| Mountains | 1–3 | Mountain symbol count (1, 2, or 3) determines Smithore bonus |

**Canonical terrain map** (approximate, matching original game):

- River plots: col 3 and col 5 (the plots adjacent to the store column on both sides) in all rows. Exact layout to be verified against original but the store column itself is the river's center.
- Mountain plots: scattered, primarily in the upper-left and lower-right quadrants. Plots with 3 mountain symbols are highest-value Smithore sites.
- Plains: all remaining plots.

Implementation note: Store the canonical terrain map as a `const char` array in ROM/static storage to save RAM.

### 4.3 Plot States

Each plot tracks:
- Owner (−1 = unowned)
- Installed MULE type (−1 = none, or commodity index 0–3)
- For-sale flag (Standard/Tournament)
- Crystite deposit level (Tournament only: 0=none, 1=low, 2=medium, 3=high)

---

## 5. Terrain and Production

### 5.1 Base Production Values

| Commodity | River | Plains | Mountains (per symbol) |
|-----------|-------|--------|----------------------|
| Food | 4 | 2 | 1 |
| Energy | 2 | 3 | 1 |
| Smithore | 0 | 1 | 1 + mountain_symbol_count |
| Crystite | N/A (no mining) | deposit-level | deposit-level |

Mountains produce Smithore equal to `1 + mountain_symbols` (so a 3-symbol mountain = 4 Smithore base).

River plots cannot produce Smithore or Crystite.

### 5.2 Production Bonuses

Two independent bonuses apply and stack:

**Adjacency bonus (Economies of Scale):**
- If 2 or more of a player's plots producing the same commodity are orthogonally adjacent (horizontally or vertically), each of those adjacent plots receives +1 production.
- Only +1 regardless of how many adjacent same-commodity plots exist.
- **Both plots must be owned by the same player.** An opponent's adjacent same-commodity plot does not qualify.

**Learning Curve bonus:**
- If a player owns 3 or more total plots producing the same commodity (anywhere on the map), all of those plots receive +1 production.
- This bonus stacks with the adjacency bonus.
- Example: 3 Smithore mines, 2 adjacent — each mine gets +1 LCT; the 2 adjacent mines each get an additional +1 EOS, for 5 total bonus units across all three plots.

**Example (6 same-commodity plots):** LCT grants 2 extra units per plot (6 ÷ 3 = 2); adjacent plots also receive +1 EOS each.

### 5.3 Energy Requirement

- Every non-Energy MULE requires exactly 1 unit of Energy from the previous turn's stock to produce anything.
- Energy MULEs use their own production as fuel; they have no external energy cost.
- If a player has insufficient Energy for their non-Energy MULEs, those MULEs produce 0 units.

**Energy deficit rule — two options (select one per implementation):**

| Option | Source | Behavior |
|--------|--------|----------|
| A (original GDD) | M.U.L.E. manual | Allocation is implementation-defined; suggest prioritize by plot index |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | For each unit of Energy the player is short, one random non-Energy-producing plot produces 0 (marked with a dark lightning bolt symbol) |

Option B is more faithful to the Planet M.U.L.E. implementation and produces fairer random outcomes.

### 5.4 Production Variance by Mode

| Mode | Production |
|------|-----------|
| Beginner | Exact base + bonuses |
| Standard | Random variance applied to base, then + bonuses |
| Tournament | Same as Standard |

**Variance model — two options (select one per implementation):**

| Option | Source | Formula | Range | Distribution |
|--------|--------|---------|-------|--------------|
| A (original GDD) | M.U.L.E. manual | `rand(0,base) + rand(0,base)` | [0, base×2] | Bell curve centered on base |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | `base + rand(-3, +3)` | [base−3, base+3] | Uniform ±3 around base |

Option A: low-base plots (base=1) can zero out frequently. Option B: variance is fixed regardless of terrain; plots with base=0 can still produce up to 3 units. Choose based on desired difficulty curve.

### 5.5 Crystite Production (Tournament)

Crystite deposit levels are fixed at game init, randomized per session (unlike terrain which is fixed).

**High-deposit count — three options (select one per implementation):**

| Option | Source | High (level 3) plots | Spread |
|--------|--------|---------------------|--------|
| A (GDD original) | GDD v1.0 | 2 | Orthogonal adjacency: Med→Low |
| B (M.U.L.E. manual) | Original M.U.L.E. manual | 3 | Same orthogonal spread |
| C (Planet M.U.L.E.) | Planet M.U.L.E. rules | 4 | Circular/radial bloom pattern |

All options: river plots have level 0 regardless of position.

- Option A/B spread: orthogonal adjacency only (cardinal directions).
- Option C spread: circular pattern — high plots radiate outward to medium, then low.

Crystite base production = deposit level (1, 2, or 3 units/turn before bonuses).

---

## 6. Commodities

### 6.1 Food

- Determines colony management time: more Food = more time during the management phase (see Section 11.1 for exact timing).
- **Spoilage:** Players can hold at most `needed + 2` units. Excess is discarded after each production phase.
- **Need:** 1 unit per installed MULE (all types). Minimum need = 1.
- **Price range:** $15–$250.

### 6.2 Energy

- Required by all non-Energy MULEs to produce (1 unit per MULE per turn).
- **Spoilage:** Players can hold at most `needed + 1` units. Excess discarded after production.
- **Need:** Count of player's non-Energy MULEs.
- **Price range:** $10–$250.

### 6.3 Smithore

- Used by the Store to manufacture MULEs (Standard/Tournament): 2 Smithore → 1 MULE, takes 1 full turn.
- **No spoilage** (see storage cap option below).
- **Price range — two options:**

| Option | Source | Beginner | Standard/Tournament |
|--------|--------|----------|---------------------|
| A (original GDD) | M.U.L.E. manual | Fixed $50 | $25–$250 |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | Fixed $50 | $35–$260 |

### 6.4 Crystite (Tournament Only)

- Shipped off-planet; no internal colony use.
- **No spoilage** (see storage cap option below).
- **Price range — two options:**

| Option | Source | Range | Mechanism |
|--------|--------|-------|-----------|
| A (original GDD) | M.U.L.E. manual | $50–$150 | Random per round, not supply/demand |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | $48–$148 | Random per round, not supply/demand |

- Only the Store buys Crystite; no player-to-player trading for Crystite.

### 6.5 Storage Limits

**Smithore and Crystite storage — two options (select one per implementation):**

| Option | Source | Smithore cap | Crystite cap |
|--------|--------|-------------|-------------|
| A (original GDD) | M.U.L.E. manual | Unlimited | Unlimited |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | 50 units/player/round | 50 units/player/round |

Option B: excess above 50 units is treated as spoilage and shown in the auction status display.

| Commodity | Max storable beyond need |
|-----------|------------------------|
| Food | +2 |
| Energy | +1 |
| Smithore | See option above |
| Crystite | See option above |

### 6.6 MULE Outfitting Costs

| Commodity | Outfitting Cost |
|-----------|---------------|
| Food | $25 |
| Energy | $50 |
| Smithore | $75 |
| Crystite | $100 |

MULE purchase cost is separate from outfitting cost.

**MULE Purchase Price — two options (select one per implementation):**

| Option | Source | Formula | Notes |
|--------|--------|---------|-------|
| A (original GDD) | M.U.L.E. manual | `BASE + ((MAX_MULES - corral_count) * SCARCITY_FACTOR) + (undeveloped_plots * DEMAND_FACTOR)` | BASE=100, SCARCITY_FACTOR=50, DEMAND_FACTOR=25; capped at $2000 |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | `2 × current Smithore price` | Price is dynamic; rises with Smithore scarcity naturally |

Option B is simpler to implement and creates a direct economic link between Smithore supply and MULE availability. Option A gives more tuning control independent of commodity prices.

**MULE Sell Refund — two options (select one per implementation):**

| Option | Source | Refund |
|--------|--------|--------|
| A (original GDD) | M.U.L.E. manual | MULE purchase price only (outfitting cost is lost) |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | MULE purchase price + outfitting cost |

---

## 7. Turn Sequence

One full turn = one round. Rounds repeat until the game ends.

```
For each round:
  1. Summary Report (net worth display)
  2. Land Grant (one free plot per player, in turn order)
  3. [Standard/Tournament only] Land Auction
  4. Colony Management (each player in turn order)
  5. Production (automatic, all plots simultaneously)
  6. [Sometimes] Pre-production Random Event
  7. Commodity Auctions (Smithore, Food, Energy, [Crystite])
  8. Post-auction wealth update
```

**Development turn order — two options (select one per implementation):**

| Option | Source | Normal order | MULE-shortage reversal |
|--------|--------|-------------|----------------------|
| A (original GDD) | M.U.L.E. manual | Poorest player moves first | N/A (consistent) |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | Leader (richest) moves first | Last-place player moves first when corral is low |

Option B is a significant strategic difference: in normal circumstances the richest player has first pick of development actions, which is a disadvantage to trailing players. The reversal compensates when MULEs are scarce.

**MULE shortage threshold for turn reversal — two options:**

| Option | Source | Threshold |
|--------|--------|-----------|
| A (original GDD) | M.U.L.E. manual | Fewer than 7 MULEs in corral → last-place moves first |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | Fewer than 8 MULEs in corral → last-place moves first |

Ties in net worth: broken by player index (lower index first).

---

## 8. Phase: Summary Report

Displayed after each round (and before the game begins).

**Content:**
- Each player's color, species, and current net worth (money + goods value + land value).
- Player ranking (1st through 4th).
- Round number / total rounds.

**Net worth formula:**
```
net_worth = cash + (sum of goods[i] * last_auction_price[i]) + (owned_plots * 500)
```

**Rank update timing:** Player rankings are updated only after the Summary phase. The leaderboard does not update mid-round. (Planet M.U.L.E. clarification.)

**Duration:** Fixed display time (approximately 3 seconds) or any button to advance.

---

## 9. Phase: Land Grant

### 9.1 Mechanics

- One free plot is available per player per round.
- Players take turns selecting from the unowned plots.
- A moving cursor (flashing square) moves across the map left-to-right, top-to-bottom.
- Player presses button to claim the currently highlighted plot.

### 9.2 Turn Order for Grants

Poorest player (lowest net worth) gets first pick. Ties: lower player index first.

### 9.3 Simultaneous Pressing

If two players press within approximately 1/10 of a second on the same plot, the poorer player wins. If tied in wealth, the player with the lower index wins.

### 9.4 UI Behavior

- The cursor moves automatically left-to-right, top-to-bottom across the map. Joystick may steer the cursor to the desired plot.
- Current plot highlighted in a neutral color; owned plots shown in owner color.
- Claimed plot flashes in winner's color.

---

## 10. Phase: Land Auction (Standard/Tournament)

### 10.1 Trigger

After the Land Grant phase, 0–6 plots may be auctioned each round. The number is randomly determined (average 1).

### 10.2 Plot Selection

A flashing black square indicates the plot to be auctioned. The plot is chosen by the game (not players).

### 10.3 Bidding

- All players press their buttons simultaneously to signal readiness.
- A minimum price is established (display it on screen).
- Each player's character moves up the screen to indicate their bid.
- Moving up = higher bid; bid increases as the character rises.
- Timer counts down.
- Dashed line shows current highest bid.

### 10.4 Result

- When timer expires: highest bidder above the minimum price wins the plot.
- If no player bids above minimum: plot goes unsold (remains unowned).
- Winner pays their bid; their character is colored to indicate ownership.

### 10.5 Plot Value in Net Worth

$500 regardless of purchase price.

---

## 11. Phase: Colony Management

### 11.1 Time Allocation

Each player has a time bar displayed on screen. Time is proportional to their Food supply.

**Management time model (Planet M.U.L.E. clarification):**
- Full food (enough for this round): **45 seconds** of management time.
- Partial food: proportional reduction. A player with 3/4 of needed food receives 3/4 of full time.
- No food: approximately **4.5 seconds** — enough to reach the Pub and end the turn.

**Implementation formula:**
```
management_ticks = BASE_TICKS + (food_owned * FOOD_TICKS_PER_UNIT)
```
Constants: `BASE_TICKS = 60`, `FOOD_TICKS_PER_UNIT = 10` (at 60 ticks/sec, Flapper receives bonus time).

Humanoid (expert species) has a time penalty: `BASE_TICKS = 40`.

When the time bar reaches zero, the turn ends immediately regardless of the player's state.

### 11.2 Town Layout

The town occupies the store column. The player character starts at the center (store entrance).

Town buildings (accessible by walking into them):

| Building | Location | Function |
|----------|----------|----------|
| Corral | Bottom center | Purchase MULEs |
| Food Outfitting Shop | Left side | Outfit MULE for Food production |
| Energy Outfitting Shop | Right side | Outfit MULE for Energy production |
| Smithore Outfitting Shop | Upper left | Outfit MULE for Smithore production |
| Crystite Outfitting Shop | Upper right | Outfit MULE for Crystite production (Tournament) |
| Pub | Lower left | End turn voluntarily for money |
| Land Office | Lower right | Mark owned plot for sale (Standard/Tournament) |
| Assay Office | Upper center | Assay a plot for Crystite deposits (Tournament) |

### 11.3 Getting a MULE

1. Walk character into Corral.
2. A MULE follows the character out.
3. Player is charged the MULE purchase price immediately.
4. If corral is empty (Standard/Tournament), no MULEs available.

**Initial MULEs in corral — two options:**

| Option | Source | Initial count |
|--------|--------|--------------|
| A (original GDD) | M.U.L.E. manual | 16 MULEs |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | 14 MULEs |

**Corral capacity — two options:**

| Option | Source | Maximum |
|--------|--------|---------|
| A (original GDD) | GDD v1.0 | 25 (implementation-defined) |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | 14 MULEs |

**Selling a MULE (refund policy) — two options (see also Section 6.6):**

| Option | Refund |
|--------|--------|
| A (original GDD) | MULE purchase price only; outfitting cost is lost |
| B (Planet M.U.L.E.) | MULE purchase price + outfitting cost both refunded |

### 11.4 Outfitting a MULE

1. Walk character (with MULE following) into the desired outfitting shop.
2. The MULE is equipped for that commodity type.
3. Outfit cost deducted from player's cash.
4. Walk character and MULE out of town to install.

### 11.5 Installing a MULE

1. Walk character with outfitted MULE off the left or right edge of the town screen onto the map.
2. Navigate to the desired owned plot.
3. When character is directly over the plot's house marker, press the button.
4. MULE is installed; production icon appears on the plot.

**Installation failure:**
- Beginner: pressing button anywhere outside the exact house position causes the MULE to run away.
- Standard/Tournament: pressing button anywhere other than directly over the house also causes MULE to escape.
- Escaped MULE: lost permanently, no refund.

**Returning MULE to corral:** Walk back to town and enter corral with MULE. Refund per Section 6.6 option selected.

### 11.6 Transferring a MULE

1. Walk character to a plot that already has a MULE installed.
2. Press button when over the house.
3. Previously installed MULE is removed and follows the player.
4. If player had a MULE already: the carried MULE is installed, the old MULE follows.
5. Player can now re-outfit the retrieved MULE or return it to the corral.

**Refit cost (Planet M.U.L.E. clarification):** When re-outfitting a retrieved MULE for a different commodity, the player pays only the difference between the new outfitting cost and the original. If the new cost is lower, the player receives a refund for the difference. Example: MULE outfit for Food ($25) re-outfit for Smithore ($75) costs $50 additional; re-outfit for Energy ($50) costs $25 additional.

### 11.7 The Pub

- Walking into the Pub immediately ends the player's management turn.
- Player receives money based on remaining time:
  ```
  pub_payment = remaining_ticks * PUB_RATE
  ```
  `PUB_RATE = 2` (dollars per remaining tick, tunable).
- More time remaining = more money earned.

### 11.8 The Wampus

- The Wampus appears randomly during management phases.
- When it opens its cave door: a bell sound plays and a light flashes on the map.
- Conditions to catch the Wampus:
  - Player must be outside town.
  - Player must NOT be carrying a MULE.
  - Player must move onto the Wampus's cell.
- If caught: Wampus pays the player a random amount (suggest $50–$150).
- Conditions preventing the Wampus from appearing:
  - Player is standing on the mountain cell the Wampus is in.
  - Player is pressing the joystick button (button press repels Wampus).
  - **Planet M.U.L.E. clarification:** The Wampus will not appear near plots with installed MULEs. Once a player installs mining equipment in a mountain area, the Wampus relocates to a quieter area. This effectively reduces Wampus opportunities as more of the map is developed.
- Wampus disappears after a short time or if not caught.

### 11.9 Land Office (Standard/Tournament)

1. Enter land office without a MULE.
2. Timer pauses while in the land office.
3. Walk to a plot you own.
4. Press button over the house marker.
5. Walk back to land office.
6. Plot is marked "For Sale" and will appear in next round's land auction.
7. Only one plot can be marked for sale per player per round.

### 11.10 Assay Office (Tournament)

**Assay cost — two options:**

| Option | Source | Cost |
|--------|--------|------|
| A (original GDD) | M.U.L.E. manual | $25 per assay |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | Free (sampling droid is provided at no charge) |

Procedure:
1. Enter assay office.
2. Walk to any plot (owned or unowned by any player).
3. Press button over the plot center.
4. Walk back to assay office.
5. Display shows Crystite level: None (0), Low (1), Medium (2), or High (3).
6. **Planet M.U.L.E. clarification:** All players briefly see the assay result when it is announced. Only the assaying player retains a persistent flag marker on that plot showing the Crystite BPV.
7. Timer continues running during assay.

### 11.11 Computer Player Management

- Computer players complete management instantly (no animation required, or brief delay for realism).
- They are prioritized as follows: Food/Energy MULEs first, then Smithore/Crystite.
- They prefer plots that maximize production bonuses (adjacency, learning curve).
- They cannot catch the Wampus.

---

## 12. Phase: Production

### 12.1 Sequence

1. For each plot with an installed MULE:
   a. Determine base production from terrain (Section 5.1).
   b. Apply production variance (Beginner: none; Standard/Tournament: see Section 5.4).
   c. Apply adjacency bonus if applicable (+1 if player owns an adjacent same-commodity plot).
   d. Apply learning curve bonus if applicable (+1 if player has ≥3 same-commodity plots).
   e. Check energy requirement: if player has insufficient energy, select affected MULEs per Section 5.3 option chosen; those MULEs produce 0.
   f. Add produced units to player's goods.

2. Apply spoilage: discard excess Food and Energy.

   **Spoilage timing — two options:**

   | Option | Source | When spoilage is applied |
   |--------|--------|------------------------|
   | A (original GDD) | M.U.L.E. manual | After production phase, before auctions |
   | B (Planet M.U.L.E.) | Planet M.U.L.E. rules | At the start of each commodity auction: usage and spoilage are subtracted first, then production is added |

3. Store rebuilds MULEs from purchased Smithore (Standard/Tournament): one MULE added to corral for every 2 Smithore units the Store purchased in the previous round.

### 12.2 Visual Display

- Each plot's production is animated: small boxes appear on the plot one at a time.
- Each box = 1 unit produced.
- Sound plays for each unit produced.

### 12.3 No-Production Conditions

A MULE produces 0 units if:
- Player lacks sufficient Energy (for non-Energy MULEs) — per Section 5.3 option selected.
- Random variance rolled 0 (Standard/Tournament, Option A) or base−3 ≤ 0 (Option B).

---

## 13. Phase: Random Events

### 13.1 Trigger

- **Player events:** One may occur per player at the start of their management turn (25% chance in Standard/Tournament; scripted in Beginner).
- **Planetary events:** One may occur before the production phase (per-round chance in Standard/Tournament).

### 13.2 Lucky/Unlucky Rule

**Bad event protection — two options (select one per implementation):**

| Option | Source | Protection from bad player events |
|--------|--------|----------------------------------|
| A (original GDD) | M.U.L.E. manual | Last place player only is never targeted |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | 3rd and 4th place players are never targeted by bad events |

**Good event protection (both sources agree):** The player in first place never receives a good luck player event.

**Targeted global event protection (Planet M.U.L.E. clarification):** When a bad planetary event targets a single player (rather than all players), it targets the 1st or 2nd place player.

**Event uniqueness (Planet M.U.L.E. clarification):** Each personal/player event can occur at most once per game. Once an event has fired, it is removed from the pool for the remainder of that game.

### 13.3 Player Random Events

Each event occurs at most once per game (Planet M.U.L.E. clarification).

| Event | Effect |
|-------|--------|
| Care Packet from Homeworld | +1 Food, +1 Energy |
| Travelling Miner | +2 Smithore |
| Gypsy In-Laws | Money loss (suggest $50–$200 range) |
| MULE Wins Prize | Small money gain (suggest $25–$75) |
| Deed Misfiled | Lose one random owned plot and its MULE |
| Relative Dies | Money loss due to inheritance taxes (suggest $100–$300) |
| Found Crystite Deposit (Tournament) | Random plot gains Crystite level upgrade |
| Lost Cargo | Lose random commodity units |
| Bonus Harvest | One random plot produces double this turn |

Implementation note: Maintain two weighted event tables (good/bad) for last-place and first-place handling.

### 13.4 Planetary Random Events

Each event has a maximum number of occurrences per game. Events become less likely with each occurrence.

| Event | Max occurrences | Effect |
|-------|----------------|--------|
| Planetary Pest | 3 | Bugs eat all food production at one plot owned by the 1st or 2nd place player (Planet M.U.L.E.); or one random Food-producing plot (original GDD). |
| Space Pirates | 2 | See pirate target option below. |
| Acid Rain Storm | 3 | All food production increased along storm path; all energy production decreased. |
| Planetquake | 3 | All Smithore and Crystite production halved this round; mountains may shift to adjacent plots, destroying MULEs on affected plots. (Planet M.U.L.E. clarification: Crystite is also halved; MULEs on shifted-mountain plots are destroyed.) |
| Sunspot Activity | 3 | All Energy production greatly increased. |
| Meteorite Strike | 2 | One random non-river plot gains a Crystite deposit (level 3); if a MULE was there, it is destroyed. |
| Radiation (MULE Goes Crazy) | 2 | One installed MULE belonging to the 1st or 2nd place player (Planet M.U.L.E. clarification) is permanently destroyed. |
| Fire in Store | 2 | All Store commodity stock is lost; MULE prices may spike. |

**Pirate attack target — two options:**

| Option | Source | Standard game target | Tournament game target |
|--------|--------|---------------------|----------------------|
| A (original GDD) | M.U.L.E. manual | All Smithore stolen | All Crystite stolen |
| B (Planet M.U.L.E.) | Planet M.U.L.E. rules | All Crystite stolen | All Crystite stolen |

Option B eliminates the Smithore-specific piracy; pirates always target Crystite in all modes.

### 13.5 Display

- Random events are announced with a text banner and associated graphic.
- Sound effect plays.
- Players have a brief moment to read the event (2–3 seconds or button press).

---

## 14. Phase: Commodity Auctions

### 14.1 Auction Order

Three auctions per round (four in Tournament):
1. Smithore
2. Food
3. Energy
4. Crystite (Tournament only)

**Auction skip (Planet M.U.L.E. clarification):** An auction for a given commodity is automatically skipped if there are no units available for trade from either any player or the Store.

### 14.2 Status Display (Pre-Auction)

Before each auction, show each player's status for that commodity:

- **Status bar:** Displays units owned → units consumed → units spoiled → units produced in the last turn.
- **Critical line:** The threshold separating surplus from shortage.
  - Food critical: amount needed for next turn (= installed MULE count).
  - Energy critical: amount needed for next turn (= non-Energy MULE count).
  - Smithore: no critical line (not survival-critical).
- **Store stock:** "Store has X units."
- **Message:** "You have a SURPLUS" or "You have a SHORTAGE."

Duration: ~2 seconds or button to advance.

### 14.3 Declare Phase

- Each player declares as BUYER or SELLER by moving joystick:
  - UP = Seller
  - DOWN = Buyer
- Default position: UP if surplus, DOWN if shortage.
- Timer counts down; players may switch sides freely until time expires.
- **Beginner:** Players with a shortage MUST be Buyers; they cannot sell below critical level.
- **Standard/Tournament:** Players may declare either way regardless of surplus/shortage.

### 14.4 Auction Phase

**Screen layout (vertical):**
```
TOP:    [Seller characters positioned here]
          --------------------------------
          |  Lowest Sell Price (dashed)  |
          --------------------------------
          |  Highest Buy Bid (dashed)    |
          --------------------------------
BOTTOM: [Buyer characters positioned here]

LEFT COLUMN: Player colors + units traded counter
RIGHT: Timer bar, Store price indicator, Units traded this auction
```

**Mechanics:**
- Sellers move DOWN to lower their asking price.
- Buyers move UP to raise their bid.
- Each player's character position indicates their current price.
- Dashed lines show: lowest ask (among all sellers) and highest bid (among all buyers).

**Price mapping:**
- Vertical screen position maps linearly to price range [min_price, max_price].
- Store's price acts as a fixed reference; the Store buys and sells at its current price.
- Players cannot set prices outside [0, max_price] where max_price = 265 for all commodities.
- Minimum prices: Food=$15, Energy=$10, Smithore=$14, Crystite=$50.

### 14.5 Trading

When the lowest seller price meets or crosses the highest buyer price:
- Both characters flash to signal a match.
- Trading begins automatically, 1 unit at a time.
- Each unit: buyer's cash decreases by the agreed price, seller's cash increases, both units-traded counters increment.
- Trade price = agreed price (buy price = sell price when they meet).
- Trading continues until:
  - Seller runs out of surplus above critical level (Beginner) or out of units (Standard/Tournament).
  - Buyer runs out of cash.
  - Either player moves away from the trade line.

**Multi-player trading:**
- A single buyer can trade with multiple sellers (or vice versa).
- When one trade partner moves away, the next closest match takes over if applicable.
- Store trades are immediate (no negotiation): if player's price matches store price, trade happens instantly.

### 14.6 Store as Market Maker

The Store has a position on the auction screen:
- Store is always a buyer if it has shortage (stock = 0).
- Store is always a seller if it has stock > 0.
- Store price changes based on supply/demand (see Section 15).
- Players cannot push the store: store's price is fixed for that auction; players must meet it.
- If store has 0 units and is only buyer: price is unbounded upward (buyers can bid any amount).

### 14.7 Auction Timer

- Fixed duration (suggest 60 seconds real-time).
- Timer displays as a bar depleting left to right.
- When timer expires, all trades stop.

### 14.8 Collusion (Tournament Only)

**Product auction collusion:**
1. Two or more players press buttons simultaneously.
2. Screen tints pink; other players and the Store fade to background.
3. A separate collusion timer begins.
4. Only colluding players can trade during this period.
5. Normal trading resumes when collusion timer expires.

**Land auction collusion:**
1. Seller presses button during land auction → selects a specific buyer.
2. Initially the leftmost buyer is highlighted.
3. Seller presses button again to cycle to the next buyer.
4. Only the highlighted buyer can bid.
5. Transaction occurs normally between seller and selected buyer.
6. If collusion timer expires without agreement, normal auction resumes.

### 14.9 Crystite Auction (Tournament)

- Crystite only trades with the Store (no player-to-player Crystite trading).
- Store price for Crystite: random in range per Section 6.4 option, set at start of each round.
- Players can hold Crystite across rounds and wait for a favorable price.
- Crystite auction is a simple: "Sell how many units?" prompt with the current Store price shown.

---

## 15. Economic Model

### 15.1 Store Pricing Formula

Store price is recalculated before each auction based on supply and demand:

```
effective_supply = store_stock + total_player_surplus_units
effective_demand = total_player_shortage_units

if effective_supply > effective_demand:
    price = max(min_price, base_price - (excess_supply * PRICE_DROP_RATE))
else if effective_demand > effective_supply:
    price = min(265, base_price + (excess_demand * PRICE_RISE_RATE))
else:
    price = base_price
```

The base_price drifts toward the equilibrium price over time (weighted average of recent auction prices).

`PRICE_DROP_RATE` and `PRICE_RISE_RATE` are tunable constants (suggest 3–5 per unit of imbalance).

Maximum store price: $265 for all commodities.

### 15.2 Initial Store Stock

**Two options (select one per implementation):**

| Commodity | Option A (M.U.L.E. manual) | Option B (Planet M.U.L.E.) |
|-----------|--------------------------|--------------------------|
| Food | 16 | 8 |
| Energy | 16 | 8 |
| Smithore | 0 | 8 |
| Crystite | 0 | 0 |
| MULEs in corral | 16 | 14 |

Option A: players must generate all Smithore themselves; no MULEs can be built from store stock until players sell Smithore.
Option B: Smithore is pre-stocked, allowing early MULE manufacturing from the start; fewer initial MULEs create earlier scarcity pressure.

### 15.3 MULE Manufacturing (Standard/Tournament)

- Each round, if the Store purchased at least 2 Smithore units in the previous auction, it converts them to MULEs.
- Rate: floor(purchased_smithore / 2) MULEs added to corral.
- MULEs do not accumulate beyond corral capacity (see Section 11.3 for capacity options).
- Building MULEs takes the Store one full round.

### 15.4 Price Floor Enforcement

No auction price can go below the commodity minimum price. No player can set their selling price below minimum; the game enforces this by clamping character position.

---

## 16. Tournament-Only Features

### 16.1 Crystite Deposits

At game start, place Crystite deposits per Section 5.5 option selected:

**Common to all options:**
- River plots have level 0 regardless of position.
- Deposits are hidden at game start; players discover via Assay Office or by installing a Crystite MULE.
- Spread pattern (Options A/B): orthogonal adjacency. High→Medium→Low.
- Spread pattern (Option C): circular radial bloom from each high-deposit center.

### 16.2 Assay Office

- Location: upper center of town (see Section 11.10).
- Reveals the Crystite deposit level of a specific plot.
- Cost: per Section 11.10 option selected ($25 or free).
- Results: all players briefly see the result; only the assayer retains a persistent flag marker.

### 16.3 Crystite Price Randomization

At the start of each round, Crystite price is set per Section 6.4 option range before the auction.

Players who hoard Crystite speculate on price fluctuations. The Pirate event (per Section 13.4 option) eliminates Crystite, creating risk.

### 16.4 Starting Resource Reduction

In Tournament mode, starting resources are reduced to the minimum necessary for one-turn survival:
- Cash: Character-tier cash (same as Standard).
- Goods: Exactly 1 Food + 1 Energy (instead of standard starting goods).

---

## 17. Scoring and Victory

### 17.1 Net Worth Formula (each player)

```
net_worth = cash
          + (food_units * food_last_price)
          + (energy_units * energy_last_price)
          + (smithore_units * smithore_last_price)
          + (crystite_units * crystite_last_price)  [Tournament only]
          + (owned_plot_count * 500)
```

If no auctions have occurred yet: use starting prices for goods valuation.

### 17.2 Victory Condition

- The player with the highest net worth after the final round wins.
- Tie: both players are declared winners.
- Player rankings are updated only after the Summary phase each round.

### 17.3 Colony Failure (Standard/Tournament)

- After each round in Standard/Tournament, sum all players' net worths.
- If combined total < $60,000: all players lose immediately; game ends.
- A failure screen is displayed.

### 17.4 Final Rankings

The winner is titled "First Founder." Second, third, fourth place receive consolation titles.

---

## 18. Computer Player AI

### 18.1 Turn Priority

Computer players evaluate actions in this order:

1. **Food/Energy self-sufficiency:** Ensure at least 1 Food MULE and 1 Energy MULE before diversifying.
2. **Smithore acquisition:** In Standard/Tournament, pursue Smithore for MULE manufacturing leverage.
3. **Crystite discovery (Tournament):** Assay promising plots; acquire high-deposit plots.
4. **Adjacency optimization:** Prefer plots adjacent to same-commodity plots already owned.
5. **Learning curve:** Aim for 3+ same-commodity plots to trigger the bonus.

### 18.2 Land Grant Strategy

- Computer calculates expected value of each plot: `terrain_production_value * remaining_rounds + 500`.
- Selects the highest-value unowned plot.
- If tied wealth with another computer player: lower index wins.

### 18.3 Auction Strategy

- Declares Seller if has surplus; Buyer if has shortage.
- As Seller: starts at maximum price, moves down only as timer runs low.
- As Buyer: starts at minimum price, moves up as timer runs low.
- Response rate is slightly slower than a human to give humans a chance.

### 18.4 MULE Management

- Always purchases a MULE if affordable and has unworked plots.
- Prioritizes Food outfit if food supply is low.
- Prioritizes Energy outfit if energy supply is low.
- Installation is instant (no visible walk sequence, or briefly animated).

---

## 19. UI Screen Specifications

### 19.1 Screen Resolution

Primary game screen: **320×200, 16-color** (NitrOS-9 screen type 8).

Text overlays: use NitrOS-9 CoWin window system, 40-column text mode over graphics.

### 19.2 Color Palette

| Palette Register | Color | Use |
|-----------------|-------|-----|
| 0 | Black | Background/border |
| 1 | White | UI text, highlights |
| 2 | Dark Green | Plains terrain |
| 3 | Blue | River terrain |
| 4 | Gray | Mountain terrain (light) |
| 5 | Dark Gray | Mountain terrain (dark) |
| 6 | Red | Player 0 color |
| 7 | Bright Blue | Player 1 color |
| 8 | Bright Green | Player 2 color |
| 9 | Yellow | Player 3 color |
| 10 | Cyan | Store/UI highlight |
| 11 | Magenta | Auction lines |
| 12 | Orange | Wampus |
| 13 | Dark Red | Negative events |
| 14 | Light Yellow | Positive events |
| 15 | Purple | Collusion screen tint |

### 19.3 Screen List

#### 19.3.1 Title Screen

- Game title "M.U.L.E." centered, large text.
- "Press any joystick button to begin" prompt.
- Background: planet landscape.

#### 19.3.2 Configuration Screens

1. **Mode Selection:** Three buttons: Beginner / Standard / Tournament.
2. **Player Count:** Slider or button list: 1–4.
3. **Color Selection:** Cycling color indicator; players press button to claim.
4. **Character Selection:** 8 character portraits in a grid; joystick navigates, button selects; selected character shown large in center.

#### 19.3.3 Main Map Screen

Layout (320×200):

```
+--------------------------------------------------+  (y=0)
| P1: $####  Food:## Enrg:## Smit:##              |  (y=0–14)  Player status bar
| P2: $####  Food:## Enrg:## Smit:##              |
+--------------------------------------------------+  (y=28)
|                                                  |
|  [9×5 plot grid, 30×28 px per plot]             |  (y=30–169)
|  Plot grid: 270 wide × 140 tall                  |
|  Centered: x=25 to 294                           |
|                                                  |
+--------------------------------------------------+  (y=170)
| Round: ##/##   [Timer bar]   [Active Player]    |  (y=172–185)
+--------------------------------------------------+  (y=186)
| [Status message area]                            |  (y=187–199)
+--------------------------------------------------+  (y=200)
```

**Plot rendering (30×28 px each):**
- Background fill: terrain color.
- Mountain symbols: small triangles (1, 2, or 3).
- Owner marker: small colored dot in corner.
- MULE installed: commodity icon (F/E/S/C letter or small sprite).
- House marker: small house icon (used for installation targeting).
- For-sale marker: "$" symbol.

**Character sprite:** 8×8 pixels, player's color.

**MULE sprite:** 8×8 pixels, neutral color.

#### 19.3.4 Town Screen

Replaces map during colony management. Same 320×200 canvas.

- Town background: simple building layout.
- Buildings labeled.
- Character sprite moves via joystick.
- MULE follows character if one is being carried.
- Timer bar displayed prominently.
- Player cash displayed.

#### 19.3.5 Summary Report Screen

- 4 player rows, each showing: color swatch, species name, net worth, land count.
- Ranking indicated by position or number.
- "Round X of Y" header.

#### 19.3.6 Status/Pre-Auction Screen

- Commodity name header.
- 4-player status bars (animated left to right showing owned/used/spoiled/produced).
- Critical line marked on each bar.
- Store stock count.
- Message text.

#### 19.3.7 Declare Screen

- Same as status screen plus joystick icons showing current BUYER/SELLER state per player.
- Timer bar.

#### 19.3.8 Auction Screen

- Full-screen vertical layout.
- Seller characters at top; buyer characters at bottom.
- Player characters are colored sprites that move vertically.
- Store character (distinct icon) at its current price position.
- Dashed horizontal lines: lowest sell price, highest buy price.
- Price scale on left edge (min to max).
- Timer bar on right edge.
- Units-traded counters for each player in corners.

#### 19.3.9 Land Grant Screen

- Map displayed.
- Flashing cursor moves left-to-right, top-to-bottom across unowned plots.
- "Press button to claim" prompt.
- Player turn indicator.

#### 19.3.10 Land Auction Screen

- Map displayed with target plot highlighted/flashing.
- Minimum price shown.
- Player character positions indicate bid level (vertical scale).
- Timer bar.
- Current highest bid shown.

#### 19.3.11 Random Event Screen

- Event name and description as large text.
- Simple graphic matching event type.
- Affected player highlighted if player event.
- Auto-advance after 3 seconds or button press.

#### 19.3.12 Production Screen

- Map displayed.
- Each plot with a MULE animates production: small boxes appear one per unit.
- Sound for each unit produced.
- Player goods totals update in real time.

#### 19.3.13 Colony Failure Screen (Standard/Tournament)

- "COLONY FAILED" banner.
- Combined net worth shown vs. $60,000 target.
- All players shown as losers.

#### 19.3.14 Victory Screen

- Player portrait and name of winner.
- "First Founder" title.
- All player net worths listed.
- "Press button to exit" or auto-exit to OS.

---

## 20. Input System

### 20.1 Joystick Mapping

The game supports up to 4 joysticks (right port = player 0, left port = player 1; ports 2–3 via software, if hardware supports it).

In a 3–4 player game: computer players substitute for missing human joystick ports.

Each joystick provides:
- X axis (0–63)
- Y axis (0–63)
- Fire button (1 or 2)

**Dead zone:** X/Y values 28–35 considered neutral (center).

**Directional mapping:**
- X < 28: Left
- X > 35: Right
- Y < 28: Up
- Y > 35: Down

### 20.2 Keyboard Fallback

- WASD or arrow keys for player 0 movement (if no joystick).
- Spacebar = button press.

### 20.3 Pause

- Spacebar pauses/unpauses the game during management phase.
- No pause during auction phases.

### 20.4 NitrOS-9 Input Method

Use `SS.Joy` GetStat call for joystick reads.  
Use `I$ReadLn` or keyboard intercept for keyboard input.  
Use `SS.KySns` for real-time key sensing (shift, arrow keys).

---

## 21. Audio

### 21.1 Sound System

Use NitrOS-9 `SS.Tone` SetStat call via VTIO path for sound output.

Parameters: frequency (relative 0–4095, higher = higher pitch), duration (ticks), amplitude (0–63).

### 21.2 Sound Events

| Event | Frequency | Duration | Notes |
|-------|-----------|---------|-------|
| MULE purchase | 400 | 5 | Brief confirmation |
| MULE installed | 500 | 10 | Ascending tone |
| Production unit | 300+n | 3 | n = unit number, rising pitch |
| MULE escaped | Descending | 15 | Falling tone |
| Wampus bell | 800 | 8 | Bell-like |
| Wampus caught | Ascending melody | 30 | |
| Auction trade | 600 | 5 | Per unit traded |
| Random event good | Ascending | 20 | |
| Random event bad | Descending | 20 | |
| Time warning | 400 | 5 | Repeated beep when <10 ticks remain |
| Game over | Descending long | 60 | |
| Victory | Victory fanfare | 90 | Multi-tone sequence |

---

## 22. Data Structures (DCC C)

All structures use DCC's 16-bit int. Use `direct` storage class for frequently accessed globals.

### 22.1 Commodity and Terrain Constants

```c
/* Commodity indices */
#define FOOD        0
#define ENERGY      1
#define SMITHORE    2
#define CRYSTITE    3
#define NUM_COMMODITIES 4

/* Terrain types */
#define TERRAIN_PLAINS   0
#define TERRAIN_RIVER    1
#define TERRAIN_MOUNTAIN 2

/* Map dimensions */
#define MAP_ROWS        5
#define MAP_COLS        9
#define STORE_COL       4    /* center column, not ownable */
#define MAX_PLOTS       44   /* ownable plots */

/* Player count */
#define MAX_PLAYERS     4

/* Game modes */
#define MODE_BEGINNER   0
#define MODE_STANDARD   1
#define MODE_TOURNAMENT 2

/* MULE states */
#define MULE_NONE       -1

/* Auction constants */
#define AUCTION_MIN_FOOD      15
#define AUCTION_MIN_ENERGY    10
#define AUCTION_MIN_SMITHORE  14
#define AUCTION_MIN_CRYSTITE  50
#define AUCTION_MAX_PRICE     265
```

### 22.2 Plot Structure

```c
typedef struct {
    signed char owner;      /* -1=unowned, 0-3=player index */
    unsigned char terrain;  /* TERRAIN_PLAINS/RIVER/MOUNTAIN */
    unsigned char mtn_count;/* 0-3: mountain symbol count */
    signed char  mule_type; /* MULE_NONE or FOOD/ENERGY/SMITHORE/CRYSTITE */
    unsigned char crystite; /* 0-3: deposit level (Tournament) */
    unsigned char for_sale; /* 0=no, 1=yes (Standard/Tournament) */
    unsigned int  sale_ask; /* asking price if for_sale */
} Plot;

/* 5x9 map, store column (col 4) is accessed but owner always -1 */
extern Plot g_map[MAP_ROWS][MAP_COLS];
```

### 22.3 Player Structure

```c
typedef struct {
    unsigned int money;
    unsigned char goods[NUM_COMMODITIES]; /* units of each commodity */
    unsigned char color;     /* 0-3 */
    unsigned char species;   /* 0-7 */
    unsigned char is_human;  /* 1=human, 0=computer */
    unsigned char plot_count;
    unsigned int  net_worth; /* updated each summary phase */
} Player;

extern Player g_players[MAX_PLAYERS];
```

### 22.4 Store Structure

```c
typedef struct {
    unsigned char stock[NUM_COMMODITIES];
    unsigned int  price[NUM_COMMODITIES]; /* current store price */
    unsigned char mule_count;             /* MULEs in corral */
    unsigned int  mule_price;
    unsigned char smithore_queued;        /* Smithore bought last round for MULE build */
} Store;

extern Store g_store;
```

### 22.5 Game State Structure

```c
typedef struct {
    unsigned char mode;           /* MODE_BEGINNER/STANDARD/TOURNAMENT */
    unsigned char round;          /* 1-based */
    unsigned char max_rounds;     /* 6 or 12 */
    unsigned char num_players;
    unsigned char num_humans;
    unsigned char turn_order[MAX_PLAYERS]; /* player indices sorted by net worth */
    unsigned char active_player;
    unsigned char auction_commodity;       /* current auction commodity */
    unsigned int  crystite_price;          /* randomized per round (Tournament) */
} GameState;

direct GameState g_state; /* direct page for fast access */
```

### 22.6 Auction State Structure

```c
typedef struct {
    unsigned char commodity;
    unsigned char is_buyer[MAX_PLAYERS];   /* 1=buyer, 0=seller */
    unsigned int  player_price[MAX_PLAYERS];
    unsigned char units_traded[MAX_PLAYERS];
    unsigned char declared[MAX_PLAYERS];    /* 1=has declared side */
    unsigned int  store_price;
    unsigned char collusion_active;         /* Tournament */
    unsigned char collusion_mask;           /* bitmask of colluding players */
} Auction;

extern Auction g_auction;
```

### 22.7 Player Position Structure (Colony Management)

```c
typedef struct {
    unsigned char x;        /* current x position in pixels or grid units */
    unsigned char y;
    unsigned char in_town;  /* 1=in town, 0=on map */
    unsigned char mule_carried; /* MULE_NONE or commodity index */
    unsigned int  time_ticks;   /* remaining management time */
} PlayerPos;

extern PlayerPos g_pos[MAX_PLAYERS];
```

### 22.8 Random Event Structure

```c
typedef struct {
    unsigned char type;     /* 0=player event, 1=planetary event */
    unsigned char subtype;  /* event index within type */
    signed char   target;   /* player index, or -1 for all */
    int           magnitude;/* variable effect size */
} RandomEvent;
```

### 22.9 Production Result Structure

```c
typedef struct {
    unsigned char produced[MAX_PLAYERS][NUM_COMMODITIES]; /* units produced per player per commodity */
    unsigned char spoiled[MAX_PLAYERS][NUM_COMMODITIES];
} ProductionResult;
```

### 22.10 Terrain Map (Static Const)

```c
/* terrain[row][col]: terrain type */
/* mtn_count[row][col]: mountain symbols (0 if not mountain) */
/* Columns 0-3 = left half; col 4 = store; cols 5-8 = right half */

static const unsigned char terrain_map[MAP_ROWS][MAP_COLS] = {
    /* Row 0 */ { PL, PL, MT, PL,  RV, PL, MT, PL, PL },
    /* Row 1 */ { MT, PL, PL, PL,  RV, PL, PL, PL, MT },
    /* Row 2 */ { PL, PL, MT, PL,  ST, PL, MT, PL, PL },
    /* Row 3 */ { MT, PL, PL, MT,  RV, MT, PL, PL, MT },
    /* Row 4 */ { PL, MT, PL, PL,  RV, PL, PL, MT, PL },
};
/* PL=Plains, MT=Mountain, RV=River, ST=Store (special) */
/* Adjust this table to match original M.U.L.E. layout exactly */

static const unsigned char mtn_count_map[MAP_ROWS][MAP_COLS] = {
    { 0, 0, 2, 0, 0, 0, 1, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 3 },
    { 0, 0, 1, 0, 0, 0, 2, 0, 0 },
    { 2, 0, 0, 3, 0, 3, 0, 0, 1 },
    { 0, 1, 0, 0, 0, 0, 0, 2, 0 },
};
/* Adjust mountain symbol counts to match original */
```

### 22.11 Key Function Signatures

```c
/* Initialization */
void game_init(unsigned char mode, unsigned char num_players);
void map_init(void);
void players_init(void);
void store_init(void);
void crystite_init(void);  /* Tournament: place deposits */

/* Turn sequence */
void phase_summary(void);
void phase_land_grant(void);
void phase_land_auction(void);   /* Standard/Tournament */
void phase_management(unsigned char player_idx);
void phase_production(ProductionResult *result);
void phase_random_event(unsigned char player_idx);
void phase_auction(unsigned char commodity);

/* Production */
int  calc_base_production(int row, int col, unsigned char commodity);
int  calc_adjacency_bonus(unsigned char player, int row, int col, unsigned char commodity);
int  calc_learning_bonus(unsigned char player, unsigned char commodity);
int  calc_production(int row, int col);
void apply_spoilage(void);

/* Economy */
void update_store_prices(void);
unsigned int calc_net_worth(unsigned char player);
void update_turn_order(void);
unsigned int calc_mule_price(void);

/* AI */
void ai_land_grant(unsigned char player);
void ai_management(unsigned char player);
void ai_auction_declare(unsigned char player, unsigned char commodity);
void ai_auction_move(unsigned char player);

/* Rendering */
void draw_map(void);
void draw_town(void);
void draw_auction(void);
void draw_summary(void);
void draw_event(RandomEvent *ev);
void draw_production(ProductionResult *result);

/* Input */
unsigned char read_joystick_x(unsigned char port);
unsigned char read_joystick_y(unsigned char port);
unsigned char read_button(unsigned char port);

/* Sound */
void play_sound(unsigned int freq, unsigned char dur, unsigned char amp);
void play_production_unit(unsigned char unit_num);
void play_victory(void);
```

---

## 23. Platform Notes: NitrOS-9 EOU / CoCo 3

### 23.1 Graphics

- Open a graphics window using DWSet with screen type 8 (320×200, 16-color).
- CoWin is required (mandated by NitrOS-9 EOU).
- Use `display` escape sequences or direct `I$Write` to the window path.
- For text overlay: use OWSet to create overlay windows on top of the graphics window.
- Use the NitrOS-9 windowing drawing commands (SetDPtr, Line, Bar, FColor, BColor, Palette) for all rendering.

### 23.2 Timing

- Clock rate: 60 ticks/second on CoCo 3 (NTSC).
- Use `F$Sleep` (via `tsleep()` in DCC) for frame timing.
- Management phase timer: count down in units of 1 tick (1/60 sec).
- Auction timer: 60 seconds = 3600 ticks.
- Use VIRQ via VRN driver (`/nil`) for 1/60-second callback if needed, or poll `F$Time`.

### 23.3 Memory Layout

- DCC places global/static data in the data segment (Y-register relative).
- Use `direct` storage class for the game state and frequently accessed globals.
- Direct page: 255 bytes available. Reserve for: `g_state` (12 bytes), hot path variables.
- The terrain map and other ROM-safe constants should be `static const` (placed in code segment, not data segment) to conserve data memory.
- Target: program + data < 64KB (single process address space for Level 2).

Estimated memory budget:
- Map data: 5×9×8 bytes (Plot struct) = 360 bytes
- Player data: 4×16 bytes = 64 bytes
- Code: ~20–30KB estimated
- Stack: 2KB minimum
- Graphics buffers: managed by GrfDrv in system map (does not consume process space)

### 23.4 Joystick Input

```c
/* Read joystick via SS.Joy GetStat */
#include <sgstat.h>

void read_joystick(unsigned char port, unsigned char *xval,
                   unsigned char *yval, unsigned char *button)
{
    struct registers r;
    r.rg_a = g_term_path;  /* path to /term or window */
    r.rg_b = 0x13;         /* SS.Joy function code */
    r.rg_x = port;         /* 0=right, 1=left */
    _os9(0x8D, &r);        /* I$GetStt */
    *xval   = r.rg_x & 0xFF;
    *yval   = r.rg_y & 0xFF;
    *button = r.rg_a;
}
```

### 23.5 Sound

```c
/* Play tone via SS.Tone on the terminal path */
void play_sound(unsigned int freq, unsigned char dur_ticks, unsigned char amp)
{
    struct registers r;
    r.rg_a = g_term_path;
    r.rg_b = 0x98;            /* SS.Tone */
    r.rg_x = ((unsigned int)dur_ticks) | (((unsigned int)amp) << 8);
    r.rg_y = freq & 0x0FFF;
    _os9(0x8E, &r);           /* I$SetStt */
}
```

### 23.6 NitrOS-9 Path Management

Open paths at startup and keep them open for the game's duration:

```c
int g_win_path;   /* graphics window path */
int g_term_path;  /* terminal path for input/sound */

/* Initialization */
g_win_path  = open("/w1", 3);     /* update mode */
g_term_path = open("/term", 3);
/* Send DWSet escape sequence to configure window */
```

### 23.7 Randomness

Use a simple 16-bit LCG (Linear Congruential Generator). Seed from system time via `F$Time`.

```c
static unsigned int g_rand_state;

void rand_seed(unsigned int seed) { g_rand_state = seed; }

unsigned int rand16(void) {
    g_rand_state = g_rand_state * 25173U + 13849U;
    return g_rand_state;
}

unsigned char rand_range(unsigned char lo, unsigned char hi) {
    return lo + (unsigned char)(rand16() % (unsigned int)(hi - lo + 1));
}
```

### 23.8 Compiler Flags

Recommended DCC invocation:
```
dcc mule.c map.c players.c auction.c ai.c render.c sound.c -m=32k -o=mule
```

Use `-s` only in performance-critical inner loops (rendering) after stack usage is verified.

Use `direct` storage class for all frequently accessed globals (game state, player data).

---

## 24. Constants Reference

Items marked with * have competing values from different sources; see the relevant section for the option table.

| Constant | Value | Notes |
|----------|-------|-------|
| MAX_ROUNDS_BEGINNER | 6 | |
| MAX_ROUNDS_STANDARD | 12 | |
| MAX_MULES_CORRAL | 25 or 14* | Option A=25, Option B=14 (Planet M.U.L.E.) — see Section 11.3 |
| INITIAL_MULES | 16 or 14* | Option A=16 (original), Option B=14 (Planet M.U.L.E.) — see Section 15.2 |
| INITIAL_STORE_FOOD | 16 or 8* | Option A=16, Option B=8 — see Section 15.2 |
| INITIAL_STORE_ENERGY | 16 or 8* | Option A=16, Option B=8 — see Section 15.2 |
| INITIAL_STORE_SMITHORE | 0 or 8* | Option A=0, Option B=8 — see Section 15.2 |
| MULE_PRICE_BEGINNER | 100 | Fixed |
| MULE_PRICE_BASE | 100 | Standard/Tournament Option A base |
| MULE_PRICE_MAX | 2000 | |
| SMITHORE_PER_MULE | 2 | |
| LAND_VALUE | 500 | Net worth contribution per plot |
| COLONY_SURVIVAL_THRESHOLD | 60000 | Standard/Tournament combined |
| FOOD_SPOILAGE_BUFFER | 2 | Extra food beyond need before spoilage |
| ENERGY_SPOILAGE_BUFFER | 1 | Extra energy beyond need before spoilage |
| SMITHORE_STORAGE_CAP | unlimited or 50* | Option A=unlimited, Option B=50 — see Section 6.5 |
| CRYSTITE_STORAGE_CAP | unlimited or 50* | Option A=unlimited, Option B=50 — see Section 6.5 |
| OUTFIT_FOOD | 25 | |
| OUTFIT_ENERGY | 50 | |
| OUTFIT_SMITHORE | 75 | |
| OUTFIT_CRYSTITE | 100 | Tournament |
| PRICE_MIN_FOOD | 15 | |
| PRICE_MIN_ENERGY | 10 | |
| PRICE_MIN_SMITHORE_STD | 14 or 35* | Option A=14 (GDD), Option B=35 (Planet M.U.L.E.) |
| PRICE_MAX_SMITHORE_STD | 250 or 260* | Option A=250, Option B=260 — see Section 6.3 |
| PRICE_MAX | 265 | All commodities auction ceiling |
| PRICE_SMITHORE_BEGINNER | 50 | Fixed |
| PRICE_CRYSTITE_MIN | 50 or 48* | Option A=50, Option B=48 — see Section 6.4 |
| PRICE_CRYSTITE_MAX | 150 or 148* | Option A=150, Option B=148 — see Section 6.4 |
| CRYSTITE_HIGH_DEPOSITS | 2, 3, or 4* | Options A/B/C — see Section 5.5 |
| MULE_TURN_THRESHOLD | 7 or 8* | Option A=7 (original), Option B=8 (Planet M.U.L.E.) — see Section 7 |
| MANAGEMENT_TICKS_BASE | 60 | At 60 ticks/sec = 1 second base (full food = 45 sec) |
| MANAGEMENT_TICKS_PER_FOOD | 10 | Additional ticks per food unit |
| MANAGEMENT_TICKS_HUMANOID | 40 | Reduced base for expert species |
| MANAGEMENT_TICKS_NO_FOOD | 27 | ~4.5 seconds at 60 ticks/sec (Planet M.U.L.E.) |
| PUB_RATE | 2 | Dollars per remaining tick |
| WAMPUS_REWARD_MIN | 50 | |
| WAMPUS_REWARD_MAX | 150 | |
| ASSAY_COST | 25 or 0* | Option A=$25, Option B=free (Planet M.U.L.E.) — see Section 11.10 |
| AUCTION_TIMER_TICKS | 3600 | 60 seconds at 60 ticks/sec |
| LAND_GRANT_CURSOR_SPEED | 2 | Pixels per tick |
| LAND_GRANT_TIE_WINDOW_MS | 100 | ~1/10 second simultaneous press window (Planet M.U.L.E.) |
| AI_TOURNAMENT_BONUS | 200 | Extra starting cash for computer in Tournament |
| PLAYER_EVENT_CHANCE | 25 | Percent chance per turn (Standard/Tournament) |
| PIRATE_OCCURRENCES_MAX | 2 | Maximum pirate events per game |
| PEST_OCCURRENCES_MAX | 3 | |
| QUAKE_OCCURRENCES_MAX | 3 | |
| SUNSPOT_OCCURRENCES_MAX | 3 | |
| METEOR_OCCURRENCES_MAX | 2 | |
| RADIATION_OCCURRENCES_MAX | 2 | |
| FIRE_OCCURRENCES_MAX | 2 | |
| ACID_RAIN_OCCURRENCES_MAX | 3 | |