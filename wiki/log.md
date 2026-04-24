# Wiki Log

Append-only chronological record of ingests, queries, and lints. Each entry prefixed `## [YYYY-MM-DD] <type> | <title>` so the log is grep-parseable.

---

## [2026-04-24] seed | Initial wiki instantiation

Created wiki scaffolding under `wiki/`:

- `CLAUDE.md` schema defining three-layer architecture (raw sources / wiki / schema), Obsidian-compatible relative-link conventions, and ingest/query/lint workflows.
- `index.md`, `log.md` root catalog + activity log.
- Source summary stubs in `sources/` for each of 9 raw source documents. GDD and AGENTS.md summarized substantively; all other references stubbed with pointers.
- Game-design pages seeded from `docs/design/MULE_GDD.md` sections 1–7, 11, 19–24: overview, modes, players, map, production, commodities, turn-sequence, management-phase, scoring. Auctions/events/economy/AI pages stubbed.
- Platform pages seeded from AGENTS.md lessons-learned and PoC source headers: stack, covdg, cowin, timing, sound, input, memory, dcc.
- Implementation pages: poc-catalog (all 7 PoCs), lessons-learned, data-structures, build-workflow, agent-workflow.

**Not yet ingested (stub pages only):**
- Full text of `mule-manual.md`, `mule-strategy-guide.md`, `Planet M.U.L.E. Rules.htm`
- All `docs/reference/*` DCC and NitrOS-9 reference manuals
- GDD sections 8–10, 12–18 (summary/land-grant/auction detail, production phase, random events, commodity auctions, economy, tournament features, AI)

Suggested next ingest: GDD §13 (Random Events) and §14 (Commodity Auctions) — these are required before implementation can begin on those phases, and current pages are stubs.
