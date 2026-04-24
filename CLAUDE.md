# Planet Pioneers

C port of the 1983 game *M.U.L.E.* for NitrOS-9 EOU on the Tandy Color Computer 3 (512K).

## Where knowledge lives

The project has an **LLM-maintained wiki at [wiki/](wiki/)** — read [wiki/index.md](wiki/index.md) first for the catalog. It is the source of truth for:

- Game design (derived from `docs/design/MULE_GDD.md`, manual, strategy guide)
- Platform / technology (CoCo 3, NitrOS-9 EOU, DCC, CoVDG, CoWin, timing, sound, input, memory)
- Implementation (PoC catalog, data structures, **lessons learned**, **build workflow**, agent workflow)
- Raw source pointers (design docs, reference manuals, PoC files)

**Always consult the wiki before answering or coding.** If a needed fact isn't there yet, read the raw source under `docs/` or `src/` and ingest the finding back into the wiki (see Wiki Maintenance below).

## Roles & Routing

Every new task begins in the `project-management` role, which classifies the task and routes to one of the specialist roles. Each role is a skill — invoke it via the Skill tool when entering that role:

- `project-management` — **mandatory first role**; classify objective/scope/affected-artifacts and pick the next role.
- `business-analyst` — ambiguous, requirement-heavy, behavior-changing, or edge-case-dependent tasks.
- `coding-architect` — module boundaries, memory/packing, runtime ownership, protocols, new technical patterns.
- `debugger` — reported error, unexpected runtime behavior, failed output, unverified regression.
- `qa-reviewer` — verification, review, regression checking, acceptance; **mandatory closing role after any implementation**.

Do not begin coding until `project-management` has run. Each role skill contains its own handoff rules and guardrails.

## Wiki Maintenance

Keeping the wiki current is part of doing work on this project — see [wiki/CLAUDE.md](wiki/CLAUDE.md) for the full schema and ingest/query/lint workflows. In brief:

- **Ingest new source files as needed.** When a task reads a raw source not yet reflected in the wiki, update `wiki/sources/` and propagate to the pages it informs.
- **Record new lessons learned.** Platform quirks, DCC gotchas, memory/packing constraints, timing findings → the relevant `wiki/platform/` or `wiki/implementation/` page.
- **Record new plans and decisions.** Capture the decision AND its rationale (especially GDD "Option A vs B" resolutions, scope changes, workflow shifts).
- **Update the index.** Any new page must be linked from [wiki/index.md](wiki/index.md).
- **Append to the log.** Every ingest, substantive query, or lint pass gets a dated line in [wiki/log.md](wiki/log.md).
- **Prefer updating over creating.** Before adding a page, check whether the concept already has one.
