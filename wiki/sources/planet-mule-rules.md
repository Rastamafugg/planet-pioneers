# Source: Planet M.U.L.E. rules

**Path:** `docs/design/Planet M.U.L.E. Rules _ Planet M.U.L.E..htm`
**Status:** STUB — not yet ingested (HTML file; needs conversion/read pass).

## What it is

Rules page from Planet M.U.L.E., a modernized implementation of the original game. The GDD uses this as **Option B** throughout its "Option A vs Option B" decision tables — a modern, more deterministic set of rules.

## Known "Option B" specifications (from GDD references)

- Starting goods: 4 Food + 2 Energy (deterministic, no cash conversion)
- Production variance: `base + rand(-3, +3)` uniform — not the original's bell curve
- MULE price: `2 × current Smithore price` (direct supply link)
- Initial MULEs in corral: 14 (vs. original 16)
- Corral capacity: 14
- Management ticks with no food: 27 ticks (~4.5 sec)
- Full-food management time: 45 seconds
- Smithore Standard/Tournament price range: $35–$260
- Crystite price range: $48–$148
- MULE shortage turn-reversal threshold: <8 in corral
- Storage cap: 50 units/player/round for Smithore and Crystite
- Energy deficit: randomly picks one non-Energy plot to zero per unit shortage (shown as dark lightning bolt)
- Simultaneous-button-press window: ~100ms (poorer player wins tie)
- Assay office: free (vs. $25)
- Crystite high-deposit count: 4 (vs. 2–3) with circular/radial bloom pattern
- Turn order: **richest first** in normal rounds; last-place first when MULEs short
- Rank update timing: only after Summary phase

## When to read

Whenever the wiki mentions an "Option A vs B" split and the user asks "what does Planet M.U.L.E. do here." Also when committing to an option per section — the GDD presents both as legitimate choices.
