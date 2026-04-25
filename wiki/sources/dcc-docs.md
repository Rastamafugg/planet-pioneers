# Source: DCC C Compiler docs

Pointers into the two DCC reference manuals. **User's Guide** is no longer a stub — it has been ingested for "modern-C-programmer gotchas." **Library Reference** is still stub-level.

## Documents

| File | Lines | Covers |
|------|-------|--------|
| `docs/reference/DCC C Compiler System Users Guide.md` | 3685 | Compiler dialect, ABI, flags, toolchain, K&R reference manual |
| `docs/reference/DCC C Compiler System Library Reference.md` | 3275 | C stdlib, NitrOS-9 system calls via `_os9()`, `<os9.h>`, `<sgstat.h>` (stub) |

Findings from the User's Guide are organized in [../platform/dcc.md](../platform/dcc.md). Lessons-learned that contradict modern-C assumptions are in [../implementation/lessons-learned.md](../implementation/lessons-learned.md).

## User's Guide — section map

Use these line ranges to revisit a topic without re-scanning 3685 lines.

### Chapter 2 — DCC characteristics (the dialect)

| Section | Lines | Why visit |
|---|---|---|
| Differences from the specification | 395–401 | Bit fields unsupported; `\n` is CR not LF; archaic `=op` rejected; init-expr arithmetic restricted to `int`/`char` operands |
| Enhancements and extensions | 402–461 | `direct` storage class (full spec); `#asm`/`#endasm`; `\l`, `\dNN`, `\xNN`, `\NNN` escapes |
| Implementation-dependent characteristics | 462–502 | Type sizes (char=1, int=2, long=4, float=4, double=8); float/double layout (NOT IEEE 754); register-variable rules (one per fn, U register); argv handling |
| System calls and the standard library | 503–520 | `pflinit`/`pffinit` referencing dance for long/float printf |
| Run-time arithmetic error handling | 521–534 | `EFPOVR`, `EDIVERR`, `EINTERR` — delivered via signal |
| Achieving maximum program performance | 535–552 | Why `long`/`float`/`double` are 4× slower; `-O` inhibits optimizer; `-P` profiler |
| C compiler components and files | 553–578 | `dcc`, `dcpp`, `dcc68`, `dco68`, `rma`, `rlink`; `clib.l`, `cstart.r` in `LIB`; `<>` includes from DEFS on default drive; `etext`/`edata`/`end` symbols |
| Running the compiler | 579–615 | `.c`/`.a`/`.r`/(none) suffixes; single-source vs multi-source mode |
| Compiler option flags | 616–633 | Full flag table including `-a`, `-r`, `-f=`, `-l=`, `-o=`, `-m=`, `-M=`, `-s`, `-O`, `-P` |
| Example command lines | 634–649 | Worked examples |

### Chapter 3 — Object code module / memory layout

| Section | Lines | Why visit |
|---|---|---|
| The object code module | 652–738 | NitrOS-9 module header generation by linker |
| Memory management | 739–830 | Default 1 KB headroom over data+strings; `-m=` selection rules |

### Chapter 4 — Basic09 interfacing (skim only)

Lines 831–1430. Cross-language ABI examples (1–6). Example 5 (1227–1374) covers float passing.

### Chapter 5 — RMA reference (skim)

Lines 1431–1725. Assembler quick reference; `psect`/`vsect`/`endsect` directives matter for `#asm` blocks (see Ch 2 line 438).

### Appendix A — Compiler error messages

Lines 1726–1862. Read on demand when an error is opaque.

### Appendix B — Compiler command lines (per-tool flag reference)

| Tool | Lines |
|---|---|
| `dcc` (executive) | 1867–1908 |
| `dcpp` (preprocessor) | 1909–1924 |
| `dcc68` (compiler) | 1925–1938 |
| `dco68` (optimizer) | 1939–1944 |
| `rma` (assembler) | 1945–1965 |
| `rlink` (linker) | 1966–1988 |

### Appendix C — C language reference (the K&R-1 reference manual)

| Section | Lines | Why visit |
|---|---|---|
| C.2 Lexical Conventions | 1995–2078 | **Identifier significance: 8 characters, 6809 external names** (line 2013); keyword list (no `void`, no `enum`); char constants are `int`; floating constants are always `double` |
| C.3 Syntax Notation + Hardware | 2079–2099 | 6809 type-size column |
| C.4 What's in a name? | 2101–2128 | Storage classes overview |
| C.6 Conversions | 2133–2176 | Usual arithmetic conversions; sign-extension rules; **at-call-site promotion (char→int, float→double)** — but see contradiction in [lessons-learned.md](../implementation/lessons-learned.md) |
| C.7 Expressions | 2177–2436 | Operator precedence; reorderability of `* + & \| ^`; argument evaluation order unspecified |
| C.8 Declarations | 2437–2792 | Storage classes (canonical list — `direct` is a DCC extension, see Ch 2); struct/union; **bit-field syntax described but UNSUPPORTED** (see Ch 2 line 397); initializers — **auto aggregates and unions cannot be initialized** (line 2683) |
| C.10 External Definitions | 2990–3051 | K&R parameter syntax; **char/short/float formal params silently widened** to int/double (line 3039) |
| C.11 Scope Rules | 3052–3088 | Lexical vs external scope; `static` at file scope hides from other files |
| C.12 Compiler Control Lines | 3089–3202 | Preprocessor; `#include "…"` searches includer's directory then standard places; `#include <…>` searches DEFS only |
| C.13 Implicit Declarations | 3203–3208 | Undeclared `id(…)` → `int id()` |
| C.14 Types Revisited | 3209–3286 | Struct assignment/pass/return supported; **no struct equality, no struct casts**; array→pointer decay; explicit pointer↔int conversion is "machine dependent" |
| C.15 Constant Expressions | 3287–3310 | What's allowed in `case`, array bounds, initializers |
| C.16 Portability Considerations | 3311–3328 | Sign extension; byte order; bit-field assignment direction |
| C.17 Anachronisms | 3329–3360 | `=op` operators; missing-`=` initializer syntax |
| C.18 Syntax Summary | 3361–3617 | Canonical grammar — note `sc-specifier` does NOT list `direct` (DCC extension, see Ch 2 line 414) |

### Appendix D — DCC vs Microware C (read in full)

Lines 3618–3677. Most quirk-level differences from "naive K&R-1" come from DCC-side improvements (preprocessor stringizing/token-paste/`__FILE__`/`#warning`/`#error`/`#line`/`#if`/`unsigned char`/`unsigned short`/`unsigned long`/`signed`; predefined `_OS9`, `__mc6809__`, `_BIG_END`; main-returns-int).

## When to read the Library Reference (still stub)

- New system call (`I$Read`, `I$SetStt`, `F$Sleep`, `F$Time`, `F$MapBlk`, `F$Mem`, etc.) or libc function.
- Concrete `struct registers` field interpretation per syscall.
- `<os9.h>`/`<sgstat.h>` macro definitions and `SS_*`/`SS.*` constants.
- `tsleep()` vs raw `F$Sleep`.

## Ingest history

- **2026-04-25** — Tier 1 ingest of User's Guide for "modern-C gotchas." Sections read: Ch 2 §§ Diff-from-spec, Enhancements, Impl-dependent, Sys-calls, Arith-errors; Appendix C §§ C.2, C.6, C.7, C.8, C.10, C.11, C.12, C.13, C.14, C.16, C.17, C.18; Appendix D in full. Findings propagated to [../platform/dcc.md](../platform/dcc.md) and [../implementation/lessons-learned.md](../implementation/lessons-learned.md).
- **2026-04-25** — Tier 2 ingest covering ABI / module layout / toolchain. Sections read: Ch 2 §§ Compiler option flags + Examples (lines 616–649); Ch 3 §§ Object code module + Memory management (lines 650–830); Appendix B per-tool flag tables for `dcc`/`dcpp`/`dcc68`/`dco68`/`rma`/`rlink` (lines 1863–1988). **Major correction:** `-s` is documented as "suppress stack-checking code," not "optimize for speed" — corrected in [../platform/dcc.md](../platform/dcc.md) and flagged in [../implementation/lessons-learned.md](../implementation/lessons-learned.md). New flags surfaced: `-2`, `-b=`, `-c`, `-d<NAME>`, `-e=`, `-lg`, `-ll`, `-ls`, `-O`/`-o` distinction, `-T`, `-n=`, `-q`. Memory layout and module format documented in `dcc.md`.
