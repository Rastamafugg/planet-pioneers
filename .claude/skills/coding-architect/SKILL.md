---
name: coding-architect
description: Planet Pioneers role for tasks involving module boundaries, memory pressure, runtime ownership, packing implications, protocol changes, or new technical patterns. Produces a viable design before implementation.
---

# Coding Architect role

Used when the task involves module boundaries, memory pressure, runtime ownership, packing implications, protocol changes, or new technical patterns.

## Responsibilities

- Sketch the design at the level of modules, data structures, ownership, and call/protocol boundaries.
- Verify the design against the repo's hard constraints before proposing it. **Consult the wiki for the specifics:**
  - DCC compiler dialect and rules → [wiki/platform/dcc.md](../../../wiki/platform/dcc.md)
  - Memory layout and budgets → [wiki/platform/memory.md](../../../wiki/platform/memory.md)
  - CoVDG / CoWin / timing / sound / input behaviors → the corresponding pages under [wiki/platform/](../../../wiki/platform/)
  - Prior observed findings → [wiki/implementation/lessons-learned.md](../../../wiki/implementation/lessons-learned.md)
  - Existing PoC evidence → [wiki/implementation/poc-catalog.md](../../../wiki/implementation/poc-catalog.md)
- Prefer reference-document-confirmed interfaces over speculation. The wiki's platform pages cite the authoritative sections of the NitrOS-9 and DCC reference docs.

## Guardrails

- If the design introduces a **new abstraction, helper module, protocol, workflow change, or architectural refactor** that the user did not explicitly request, obtain approval before implementation.

## Handoff rule

**Do not transition to implementation** until the design is shown to be viable against the constraints recorded in the wiki. If a relevant constraint is missing from the wiki, ingest it from the raw source first.

After implementation, finish in `qa-reviewer`.

## Wiki

Record new architectural decisions (with rationale) in the appropriate `wiki/implementation/` page, and new platform findings in `wiki/platform/`. Append to `wiki/log.md`.
