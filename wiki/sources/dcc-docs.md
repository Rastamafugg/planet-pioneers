# Source: DCC C Compiler docs

**Status:** STUBS — not yet deeply ingested.

## Documents

| File | Lines | Covers |
|------|-------|--------|
| `docs/reference/DCC C Compiler System Users Guide.md` | 3685 | Invocation, flags, driver, linker, toolchain |
| `docs/reference/DCC C Compiler System Library Reference.md` | 3275 | C stdlib, NitrOS-9 system calls via `_os9()`, `<os9.h>`, `<sgstat.h>` |

## When to read

- **Library ref** when the implementation or a PoC needs a new system call (`I$Read`, `I$SetStt`, `F$Sleep`, `F$Time`, `F$MapBlk`, `F$Mem`, etc.) or a libc function. GDD §23 pulls examples from this (e.g. `tsleep`, `_os9` with `struct registers`).
- **User's guide** when a build or link problem appears, when considering `-s` (speed) vs default, when using `-m=` stack-size flag, or when using the `direct` storage class.

## Known key facts from AGENTS.md lessons

- DCC is K&R-style: declarations at block start, avoid `void` prototypes, avoid modern C syntax, keep external symbol names short/distinct.
- `direct` storage class available for direct-page globals — recommended for `g_state` and hot-path variables.
- Recommended compile flags from GDD §23.8: `dcc *.c -m=32k -o=mule`; reserve `-s` for verified inner loops.

## Ingest priorities (when touched)

1. `struct registers` and `_os9()` calling convention.
2. `tsleep()` vs raw `F$Sleep`.
3. Available GetStat/SetStat constants (SS_*).
4. Memory map flags passed to `_os9` for `F$Mem` / `F$SRqMem` / `F$MapBlk` / `SS.ARAM`.
5. Linker / module build (`-f=/dd/cmds/...`, `-m=` stack size selection).
