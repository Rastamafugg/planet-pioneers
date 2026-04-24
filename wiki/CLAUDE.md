# Wiki Schema & Workflow

This directory is an LLM-maintained wiki for the **Planet Pioneers** project — a C port of the 1983 game *M.U.L.E.* for NitrOS-9 EOU on the Tandy Color Computer 3. The wiki compiles knowledge from the repo's raw sources (`docs/`, `src/`, `AGENTS.md`) into a persistent, interlinked set of markdown pages so future sessions don't have to re-derive everything.

## Layering

- **Raw sources** — `../docs/`, `../src/`, `../AGENTS.md`, `../images/`. Immutable; the wiki reads from them but never edits them.
- **Wiki** (this directory) — everything here is LLM-written. Seed content came from a first-pass ingest of the GDD and PoC source files.
- **Schema** (this file) — describes structure and workflows.

## Directory layout

```
wiki/
├── CLAUDE.md              — this file
├── index.md               — content catalog (read first when answering a query)
├── log.md                 — chronological append-only log of ingests/queries/lints
├── sources/               — one page per raw source document, with summary + pointers
├── game/                  — game design: entities, phases, mechanics (derived from GDD, manual, strategy guide)
├── platform/              — technical platform: CoCo 3, NitrOS-9, DCC, graphics/sound/timing subsystems
└── implementation/        — project-specific: PoC catalog, data structures, lessons learned, build workflow
```

Pages link each other with relative markdown links (e.g. `[Production](../game/production.md)`), not Obsidian `[[wikilinks]]`, so they render in both Obsidian and plain markdown viewers.

## Page conventions

- Each page starts with a one-sentence **purpose** line, then content.
- Cite the raw source at the bottom under `## Sources`: e.g. `- docs/design/MULE_GDD.md §5.2` or `- src/c/poc_cvdg16.c:L40-60`.
- When a claim has competing values between sources (the GDD lists many "Option A vs Option B" choices), record BOTH with their sources — don't silently pick one.
- Prefer small interlinked pages over long monolithic ones. If a page exceeds ~300 lines consider splitting.
- Frontmatter (optional) for pages that are queryable via Dataview:
  ```
  ---
  type: entity | concept | source | finding
  tags: [game, economy, ...]
  updated: 2026-04-24
  ---
  ```

## Workflows

### Ingest (new source)

1. Read the source.
2. Discuss key takeaways with the user before writing (unless they said "just file it").
3. Write/update the source summary page under `sources/`.
4. Propagate: update every entity/concept page the source touches. A single ingest usually touches 5–15 pages.
5. Update [index.md](index.md) with any new pages.
6. Append a line to [log.md](log.md): `## [YYYY-MM-DD] ingest | <Source Title>` + one paragraph on what changed.

### Query

1. Read [index.md](index.md) first — it's the map.
2. Drill into relevant pages. If you need to, fall back to reading raw sources in `../docs/` or `../src/`.
3. Synthesize the answer with citations (wiki page + raw source).
4. If the answer contains new synthesis worth keeping (a comparison, a derived table, a decision rationale), **file it back** as a new wiki page under the appropriate folder and link it from the index. Don't let valuable exploration disappear into chat history.
5. Append to [log.md](log.md): `## [YYYY-MM-DD] query | <one-line question>` + one line on the answer or page created.

### Lint

Periodically, scan for:
- Contradictions between pages (especially around the GDD's "Option A vs B" splits — is the same option consistently chosen across pages?).
- Orphan pages (no inbound links).
- Concepts mentioned but lacking their own page.
- Stale claims where code has since moved on from a PoC finding.
- Raw sources that have not yet been ingested.

Record findings in `log.md` and fix what's cheap; surface the rest as a todo list for the user.

## Project-specific ingest priorities

At time of seeding (2026-04-24), these raw sources exist but have **not been deeply ingested** — pages summarize them at a surface level only:

- `docs/design/mule-manual.md` (998 lines) — original M.U.L.E. manual prose
- `docs/design/mule-strategy-guide.md` (449 lines) — strategy tips
- `docs/design/Planet M.U.L.E. Rules _ Planet M.U.L.E..htm` — the "Option B" source referenced throughout GDD
- `docs/reference/DCC C Compiler System Library Reference.md` (3275 lines)
- `docs/reference/DCC C Compiler System Users Guide.md` (3685 lines)
- `docs/reference/NitrOS-9 EOU Operating System User's Guide.md` (4416 lines)
- `docs/reference/NitrOS-9 EOU Technical Reference.md` (9670 lines)
- `docs/reference/NitrOS-9 EOU Level 2 Windowing System Manual.md` (1385 lines)

Each has a stub page in `sources/`. When the user asks a question that requires one of these, do a targeted read of the relevant section and expand both the source page and any platform/game pages it informs. Do not attempt to ingest all of them up front.

The GDD (`docs/design/MULE_GDD.md`) has been **substantively ingested** for sections 1–7, 11, 19–24 and spot-checked elsewhere. Sections 13–18 (random events, auctions, economy, tournament features, AI, scoring) are stub-level and are high-value targets for the next ingest pass.

## Things to prefer

- **Link early.** When you mention a concept that has (or should have) its own page, link it even if the page is a stub.
- **Record "why".** Design documents list dozens of "Option A vs Option B" decisions. When you learn which one the codebase is implementing, write the choice AND the rationale.
- **Keep the index tight.** One line per page under ~150 chars. The index is always in context; long-form belongs in the page.
- **Prefer updating over creating.** Before adding a page, grep for the concept in existing pages.
