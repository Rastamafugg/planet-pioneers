# Source: AGENTS.md

**Path:** `AGENTS.md` (94 lines)
**Status:** Fully ingested.

## What it is

The project's top-level agent operating manual. Defines a role-based workflow and carries the current set of platform lessons learned from PoC work.

## Roles and routing

Every task opens in **Project Management** role, which classifies the task and routes to one of:

- **Business Analyst** — ambiguous / requirement-heavy / behavior-changing / edge-case clarification needed
- **Coding Architect** — module boundaries, memory pressure, runtime ownership, packing, protocol changes, new technical patterns
- **Debugger** — reported error, unexpected runtime, failed output, unverified regression
- **(direct impl)** — only when change is narrow, approved, and specified
- **QA Reviewer** — verification, review, regression, acceptance

Every implementation task **finishes in QA Reviewer**. Gating rules prevent skipping to code from BA (requirements must be explicit), from CA (design must be viable for memory/packing/runtime), or from Debugger (must reduce to observed facts + discriminating cause unless user approves hypothesis-driven fix).

Unrequested abstractions, helper modules, protocol changes, workflow changes, or architectural refactors **require approval** before implementation.

See [implementation/agent-workflow.md](../implementation/agent-workflow.md) for the practical version.

## Project facts captured here

- C-based M.U.L.E. implementation targeting **NitrOS-9 EOU on Tandy CoCo 3 with 512K RAM**.
- **Primary design source:** `docs/design/MULE_GDD.md`.
- Reference documents for language/OS: DCC Library Reference, DCC User's Guide, NitrOS-9 EOU User's Guide, Technical Reference, Level 2 Windowing System Manual.
- `images/` contains C64-era screenshots for visual reference.

## Build workflow (see also [build-workflow.md](../implementation/build-workflow.md))

- `src/script/buildc` — full rebuild. Must contain one `dcc` command per C source file in `src/c`.
- `src/script/patchc` — fast iteration. After each task, reduced to only the changed files. Should be minimal and task-specific.
- `disks/ppsrc.dsk` — **owned by the deploy/build workflow**; do not modify during normal source/script edits. Touch only when the task is explicitly about disk-image contents.

## PoC code location

- Active PoC / current program source: `src/c/`.
- Verified PoCs should be moved to `src/poc/` for reference (not created yet).

## Lessons learned captured (platform/runtime)

All listed under AGENTS.md §66+ and propagated to [platform/](../platform/) and [implementation/lessons-learned.md](../implementation/lessons-learned.md):

- DCC is K&R-era: declarations at block start, avoid `void` prototypes, avoid modern C, keep external symbols short.
- CoWin GET/PUT: `GetBlk`/`PutBlk` fast enough for small sprites when buffer = background-union-sprite; one 40×40 union buffer per direction beats separate background restore + sprite put; byte-aligned X where possible.
- CoWin has **no true page flip or guaranteed VBlank sync**. Command buffering only.
- VRN `/nil` `SS.FSet` provides a documented 1/60s timing gate — measurement-grade, not tear-free-sync-grade.
- CoVDG: call `SS.AScrn`/`SS.DScrn`/`SS.FScrn` on a VTIO/CoVDG VDG path, not a CoWin window. `/verm` accepts `SS.AScrn` where `term_vdg`/`TERM_VDG` did not. `SS.DScrn` only displays when the CoVDG path is the current interactive device; send CoVDG `Select` (`ESC $21`) first. Display screen 0 before freeing allocated high-res screens.
- CoVDG memory: type 4 (320×192×16, 32K) — a second 32K allocation failed with error #207. Type 2 (160×192×16, 16K) is the viable page-flip PoC target.
- `F$Time` is coarse (whole seconds); unexpectedly slow timing should be treated as an observed fact pending a discriminating test.

## Sources

- `AGENTS.md`
