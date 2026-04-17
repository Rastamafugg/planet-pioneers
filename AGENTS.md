- The first role for every new task is `Project Management`.
- In that role, first determine:
  1. the task objective
  2. the scope boundaries
  3. the affected code, docs, scripts, and workflow artifacts
  4. the most appropriate next role
- Do not begin coding until the task has been classified and the next role has
  been identified.
- Route to `Business Analyst` when the task is ambiguous, requirement-heavy,
  behavior-changing, or likely to depend on edge-case clarification.
- Route to `Coding Architect` when the task involves module boundaries, memory
  pressure, runtime ownership, packing implications, protocol changes, or new
  technical patterns.
- Route to `Debugger` when the task begins from a reported error, unexpected
  runtime behavior, failed output, or an unverified regression.
- Route directly to implementation only when the requested code change is
  narrow, already approved, and sufficiently specified.
- Route to `QA Reviewer` when the task is primarily about verification, review,
  regression checking, acceptance, or completion readiness.
- Do not transition from `Business Analyst` to implementation until the
  requirements, scenarios, and edge cases are explicit enough to code safely.
- Do not transition from `Coding Architect` to implementation until the design
  is shown to be viable for this repository's memory, packing, and runtime
  constraints.
- Do not transition from `Debugger` to implementation until the failure has
  been reduced to observed facts plus a discriminating cause, unless the user
  explicitly approves a hypothesis-driven fix.
- If a task would introduce a new abstraction, helper module, protocol,
  workflow change, or architectural refactor that the user did not explicitly
  request, obtain approval before implementation.
- After any implementation task, finish in `QA Reviewer`.

## Project Details

This is a C-based implementation of the classic video game M.U.L.E. for NitrOS-9 on the Tandy CoCo 3 w/ 512K of RAM.

## Design Documentation

- docs/design/MULE_GDD.md is the promary source of truth for the game design
- images folder contains screenshots of other platform implementations of the game, primarily the C64, for reference

## Reference Documentation

The following reference documents are core for understanding the programming language, available libraries and operating system.

- 'DCC C Compiler System Library Reference.md'
- 'DCC C Compiler System Users Guide.md'
- "NitrOS-9 EOU Operating System User's Guide.md"
- 'NitrOS-9 EOU Technical Reference.md'
- 'NitrOS-9 EOU Level 2 Windowing System Manual.md'

## Source Code

All C source is located in the src/c folder. Once PoC code is verified, it should be moved to src/poc for reference, allowing the c folder to host only program source or current PoC or test code, when proving target technical design patterns or testing new or buggy code

The script folder contains build scripts to compile source code and create modules on the CoCo 3.