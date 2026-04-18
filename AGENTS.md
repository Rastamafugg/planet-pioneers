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

## Build Scripts

- `src/script/buildc` is the full C rebuild script. Keep it updated with one `dcc` command for every C source file in `src/c`.
- `src/script/patchc` is the fast deployment/test script. After each source change, update it to contain only the `dcc` commands for C sources changed in that task.
- Remove unchanged build calls from `src/script/patchc` whenever updating it. The intent is that `patchc` remains minimal and task-specific, while `buildc` remains complete.
- Do not update `disks/ppsrc.dsk` during normal source or script edits. The deploy/build workflow owns copying changed files into the disk image.
- Only modify `disks/ppsrc.dsk` when the user explicitly requests disk-image changes or the task is specifically about disk-image contents.

## NitrOS-9 / CoCo 3 Lessons Learned

- Treat DCC as an old K&R-style C compiler:
  - keep declarations at the start of blocks
  - avoid `void` prototypes
  - avoid modern C syntax
  - keep external symbol names short and distinct to reduce linker/name-collision risk
- After changing source files used by the emulator workflow, leave repository files as the source of truth and rely on the deploy/build workflow to update the OS-9 disk image.
- Prefer reference-document-confirmed interfaces before changing PoC code. Use the NitrOS-9 EOU Technical Reference, Level 2 Windowing System Manual, and DCC references as primary sources.
- CoWin `GET/PUT` findings:
  - `GetBlk` / `PutBlk` can be fast enough for small sprite tests when the buffer contains the union of restored background plus new sprite
  - one 40x40 union buffer per movement direction reduced flicker better than separate background restore plus sprite put
  - use byte-aligned X coordinates where possible; prior tests used 8-pixel steps successfully
  - CoWin command buffering does not provide true page flipping or guaranteed VBlank synchronization
- CoWin timing findings:
  - user-level VRN `/nil` `SS.FSet` can provide a documented 1/60-second timing gate for measurement
  - this is not direct GIME VBlank/HSync access and should not be described as guaranteed tear-free synchronization
- CoVDG screen findings:
  - call `SS.AScrn`, `SS.DScrn`, and `SS.FScrn` with `I$SetStt` on a VTIO/CoVDG VDG path, not a CoWin window
  - `/verm` has been observed to accept `SS.AScrn` where `term_vdg` and `TERM_VDG` did not
  - `SS.DScrn` only displays if that CoVDG path is the current interactive device
  - send CoVDG `Select` (`ESC $21`) to the opened VDG path before expecting `SS.DScrn` output to become visible
  - screen 0 should be displayed before freeing allocated high-resolution screens
- External screen memory findings:
  - 320x192x16 CoVDG screen type 4 requires 32K and a second 32K allocation failed with error #207 in testing
  - 160x192x16 CoVDG screen type 2 requires 16K and is the better PoC for testing whether page flipping works when memory fits
- Timing diagnostics:
  - `F$Time` returns only whole seconds, so FPS/timing output is coarse
  - if a test reports unexpectedly slow timing, treat that as an observed fact and avoid assuming the cause without a discriminating test
