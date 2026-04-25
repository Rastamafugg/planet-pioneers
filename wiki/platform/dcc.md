# DCC Compiler Notes

DCC is the C compiler targeting 6809/6309 under NitrOS-9. It is an **old K&R-style compiler** — modern C idioms will not compile.

## Writing-DCC-friendly C (from AGENTS.md)

- **Declarations at the start of each block** — no mid-block declarations.
- **Avoid `void` prototypes** — use empty `()` for "no args" (K&R style) rather than `(void)`.
- **Avoid modern C syntax** — no compound literals, no C99+ features.
- **Keep external symbol names short and distinct** — linker / name-collision risk with long identifiers.
- **16-bit int** — DCC's `int` is 16 bits. Use `unsigned int` for 0–65535 counts and addresses. Use `long` (if supported) for larger ranges.

Every PoC in `src/c/` follows these conventions; they are living examples.

## Compiler invocation

Recommended starter from GDD §23.8:

```
dcc mule.c map.c players.c auction.c ai.c render.c sound.c -m=32k -o=mule
```

## Flags seen in PoCs

| Flag | Meaning | Used in |
|------|---------|---------|
| `-s` | Optimize for speed (inner loops) | **Only after** stack usage verified. PoCs use for performance |
| `-m=4k`, `-m=8k`, `-m=24k`, `-m=32k` | Stack size | Per-PoC tuning |
| `-f=/dd/cmds/<name>` | Output module path | All PoCs |

Example from [poc_cvdg16.c](../sources/poc-sources.md):

```
dcc poc_cvdg16.c -s -m=4k -f=/dd/cmds/poccvd16
```

## Storage classes

- `direct` — supposed to place a global in the direct-page (255 B pool) for single-byte addressing. ⚠ **Unsafe as written**: a bare `direct GameState g_state;` in [main.c](../../src/c/main.c) crashed EOU on first `printf` (2026-04-25), probably from collision with libc's own direct-page slots. Treat `direct` as an unverified PoC target until a working pattern is found. See [memory.md](memory.md) and [implementation/lessons-learned.md](../implementation/lessons-learned.md).
- `static` — file-local scoping; also avoids symbol-table pressure.
- `static const` — constant data in code segment; doesn't consume data RAM.

## OS interaction — `_os9` and `struct registers`

System calls go through `_os9()`:

```c
#include <os9.h>

struct registers r;
r.rg_a = path;
r.rg_b = SS_TONE;                 /* 0x98 */
r.rg_x = dur | (amp << 8);
r.rg_y = freq & 0x0FFF;
_os9(0x8E, &r);                   /* I$SetStt */
```

The call code constants (`I$SetStt = 0x8E`, `I$GetStt = 0x8D`, `F$Sleep = 0x0A`, `F$Time = 0x15`) are often re-`#define`d in PoCs when the headers don't supply them — see PoC file headers.

## Libc subset (`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<os9.h>`, `<sgstat.h>`)

Used across PoCs. Detailed function list lives in `docs/reference/DCC C Compiler System Library Reference.md` ([stub](../sources/dcc-docs.md)).

## Sources

- `AGENTS.md` DCC lessons
- `docs/design/MULE_GDD.md` §23.8
- `docs/reference/DCC C Compiler System Users Guide.md` (stub)
- `docs/reference/DCC C Compiler System Library Reference.md` (stub)
