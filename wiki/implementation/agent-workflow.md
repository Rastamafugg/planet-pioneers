# Agent Workflow (from AGENTS.md)

AGENTS.md §1–31 defines a **role-based operating model**. Every task must pass through a classification step before any code is touched.

## The required opening move

> "The first role for every new task is `Project Management`."

In PM role, determine:
1. Task objective
2. Scope boundaries
3. Affected code, docs, scripts, and workflow artifacts
4. The most appropriate next role

**Do not begin coding until the task has been classified and the next role has been identified.**

## Routing table

| Route to | When the task is… |
|----------|------------------|
| **Business Analyst** | Ambiguous / requirement-heavy / behavior-changing / depends on edge-case clarification |
| **Coding Architect** | Module boundaries / memory pressure / runtime ownership / packing implications / protocol changes / new technical patterns |
| **Debugger** | Reported error / unexpected runtime behavior / failed output / unverified regression |
| **(direct impl)** | Narrow, already approved, sufficiently specified code change |
| **QA Reviewer** | Verification, review, regression checking, acceptance, completion readiness |

## Gating rules

- **BA → impl:** only when requirements, scenarios, and edge cases are explicit enough to code safely.
- **CA → impl:** only when the design is shown viable for this repo's memory, packing, and runtime constraints (see [memory.md](../platform/memory.md), [lessons-learned.md](lessons-learned.md)).
- **Debugger → impl:** only when the failure is reduced to **observed facts + a discriminating cause**. Exception: user explicitly approves a hypothesis-driven fix.
- **Finish:** every implementation task ends in QA Reviewer.

## Approval required before adding

If a task would introduce **any of the following** that the user did not explicitly request, obtain approval before implementation:

- New abstraction
- Helper module
- New protocol
- Workflow change
- Architectural refactor

This is an important cultural rule — the project values **minimal, scoped change** over plausibly-useful expansion.

## Why this matters for the wiki

Ingesting new knowledge that **changes wiki structure or conventions** (beyond adding a page or updating content) should itself go through the same PM classification — ask before reorganizing folders, renaming core pages, or introducing new page categories.

## Sources

- `AGENTS.md` §1–31
