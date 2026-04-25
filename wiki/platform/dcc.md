# DCC Compiler Notes

DCC is the C compiler targeting 6809/6309 under NitrOS-9. Its dialect is **K&R First Edition** with selected modern additions (per Appendix D of the User's Guide), and a handful of platform-specific extensions. Modern (C99/C11) C idioms will not compile, and even some K&R-1 constructs behave non-obviously on the 6809.

This page is the canonical "what you need to know about DCC to write code for it." Every claim cites the raw doc; line numbers refer to `docs/reference/DCC C Compiler System Users Guide.md` ("UG") unless stated.

> The companion page [../implementation/lessons-learned.md](../implementation/lessons-learned.md) records the *non-obvious gotchas* — things that bit us on PoCs and that a modern C programmer would not predict from reading the spec alone.

## Language conventions (K&R-1 dialect)

- **Declarations at the start of each block.** Mid-block declarations are not accepted (K&R-1 grammar; UG § C.18, lines 3539–3571).
- **K&R function-definition syntax only.** Parameter names go in parens; types are declared *between* the declarator and the body. No prototype syntax. UG § C.10.1, lines 2990–3037 and example at 3026:
  ```c
  int max(a, b, c)
  int a, b, c;
  { ... }
  ```
- **No `void` keyword.** The keyword list in UG § C.2.3 (lines 2017–2030) does not include `void`. Use empty `()` for "no args" rather than `(void)`. Functions returning nothing return `int` by default; use `char *` instead of `void *`.
- **No `enum` keyword either.** Use `#define` constants.
- **`signed` IS supported** (Appendix D, line 3656) — `signed char` works.
- **Identifier significance: 8 characters, case-sensitive.** From UG § C.2.2 (lines 2005–2013): non-external identifiers significant for "at least 8 characters"; **6809 external names: 8 characters, 2 cases**. This is the documented basis for the linker collisions we have already seen — see [lessons-learned.md](../implementation/lessons-learned.md). Keep external (non-`static`) symbol distinctness within the **first 8 characters**.
- **Bit fields are not supported.** UG line 397, in "Differences from the specification." Note that bit-field *syntax* is described in UG § C.8.5 / § C.18.2 — that's the K&R reference manual; defer to Chapter 2's explicit non-support statement.
- **Initializer arithmetic is restricted to `int`/`char` operands** (UG line 398). `int x = 1L + 2L;` won't fold at compile time.
- **Archaic `=op` operators (`=+`, `=-`, etc.) not accepted** — only modern compound forms `+=`, `-=` (UG line 399). Anachronism per § C.17.
- **`\n` is `\r` (0x0D), not LF (0x0A).** OS-9 uses CR for end-of-line (UG line 400). DOS/Windows-style `\r\n` will print **two carriage returns**. Use `\l` (the DCC extension at UG line 445) when an actual LF is required.
- **Extra escape sequences:** `\l` (LF, lower-case ell), `\NNN` (octal — standard), `\dNN` (decimal — DCC-specific), `\xNN` (hex — standard). UG lines 440–460. The decimal escape is non-standard.
- **Auto aggregates / unions cannot be initialized.** UG line 2683: "It is not permitted to initialize unions or automatic aggregates." A `struct Foo f = { 1, 2 };` at function scope is rejected; you must zero or assign at runtime, or make it `static`.
- **Static and external data are auto-zeroed; auto and register data are garbage.** UG line 2679. Cannot rely on auto vars being zero.
- **String literals are distinct.** UG line 2073: "All strings, even when written identically, are distinct." Don't depend on string-literal pooling.
- **Character constants have type `int`** (UG line 2206). Floating constants are always `double` (line 2206/2067).

## Type system

### Sizes and representations

| Type | Bytes | Form (UG line 470–477) |
|------|-------|------------------------|
| `char`, `unsigned char` | 1 | two's complement / unsigned binary |
| `short`, `int`, `unsigned int` | 2 | `short == int` on 6809 |
| `long`, `unsigned long` | 4 | two's complement |
| `float` | 4 | DCC custom format — **not IEEE 754** |
| `double`, `long double` ≡ `double` | 8 | DCC custom format |

`int` is 16-bit. Plan address ranges and counts accordingly. Use `unsigned int` for 0–65535 ranges. UG § C.6.1 (line 2139) — `char` is sign-extended to `int` (PDP-11 convention); `unsigned char` widens without sign change.

### Float / double layout (UG lines 481–491)

```
(low addr)                                       (high addr)
| sign+55-bit mantissa (sign+magnitude, implied 1) | 8-bit exp |
```

Exponent biased by 128. `float` is the same with 3-byte mantissa instead of 7. **Do not exchange binary float data** with anything that expects IEEE 754 — the encodings differ.

### "Use of `long`/`float`/`double` is 4× slower and 4× larger" (UG line 539)

> Don't use `long`, `float`, or `double` where an `int` or `unsigned int` will do.

The compiler can fold constant expressions involving `int`/`char` only — `long`/`float`/`double` constant expressions are evaluated at run time (UG line 541). Manually pre-compute them.

### `pflinit` / `pffinit` referencing dance (UG line 519, **major gotcha**)

By default, `printf` does **not** include `long` or `float`/`double` formatters — to save memory. To pull in formatting code:

```c
extern pflinit(); /* somewhere reachable, even unused */
extern pffinit();
```

A reference to `pflinit` (long support) or `pffinit` (float support) anywhere in the program tells the linker to include the relevant routines. Without the reference, `%ld` / `%f` will silently misformat.

## Storage classes

Canonical list (UG § C.8.1, line 2461): `auto`, `static`, `extern`, `register`, `typedef`. DCC adds **`direct`** (UG § "The direct storage class," lines 404–432).

### `auto`, `static`, `extern`, `register`

- Default inside a function: `auto`. Outside a function: `extern` (UG line 2475).
- Functions can never be `auto`.
- **Only one `register` variable per function** (UG line 495). Allowed types: `int`, `unsigned int`, pointer. Other types or extra `register` declarations are silently downgraded to `auto`. The U register is used for the register variable.
- The `&` (address-of) operator cannot be applied to a `register` variable.

### `direct` (DCC extension)

DCC-specific storage class that places a global in the 6809's direct page (256 bytes addressable with a 2-byte instruction instead of 4-byte). UG lines 404–432. Forms: `direct`, `static direct`, `extern direct`. The User's Guide states explicitly that:

- **255 bytes total are available** on the direct page (linker reserves 1 of the 256).
- **Library functions do not use `direct`** (UG line 430) — so user direct-page variables coexist with libc's own globals.
- **`direct` cannot be used for function arguments** (UG line 428).
- Initializers behave like other storage classes; uninitialized = zero.
- If too many `direct` bytes are requested, the linker errors out and the programmer must reduce them.
- **Not portable.** UG line 432: "direct is unique to this compiler, and it may not be possible to transport programs written using the direct storage class to other environments."

The `direct` keyword does **not** appear in the Appendix C grammar (UG § C.18.2, line 3458); it is a Chapter 2 extension over the canonical K&R-1 spec. Status in our code: see [lessons-learned.md](../implementation/lessons-learned.md) — no PoC currently uses it.

## Conversions / ABI / argument passing

### Spec-claimed conversions at call sites (UG § C.7.1, line 2216, and § C.10.1, line 3039)

The reference manual says:
- `float` actual parameters → `double` before the call.
- `char` / `short` actual parameters → `int` before the call.
- "**No other conversions are performed automatically; in particular, the compiler does not compare the types of actual arguments with those of formal arguments.**"
- Formal `char` / `short` parameter declarations are silently adjusted to `int`; `float` adjusted to `double` (UG line 3039).

### What we have actually observed

The spec text says char→int promotion happens. **Our PoCs disagree** for varargs / unprototyped calls — passing `unsigned char` to `printf("%d", …)` reads stack garbage as the high byte. See [lessons-learned.md](../implementation/lessons-learned.md) for the contradiction and a tentative resolution (the size-not-type ABI dominates for `…` varargs, since there's no formal parameter to drive promotion). **Treat call-site promotion as unreliable on this compiler. Always cast `char` → `(int)` at the call site for variadic and unprototyped functions.**

### Other ABI facts

- "All argument passing in C is strictly by value" — UG line 2218.
- **Order of argument evaluation is unspecified** (UG lines 2218, 3321). Don't rely on left-to-right or right-to-left.
- Expressions involving `* + & | ^` may be **rearranged even in the presence of parentheses** (UG line 2181). To force order, use a temporary.
- Pointer-to-int conversions are "machine dependent" but on 6809 a pointer fits in `int` (16 bits).

## Preprocessor

DCC's preprocessor (`dcpp`) is **more modern than K&R-1 alone implies** — Appendix D (lines 3632–3650) lists ANSI features that have been retrofitted:

- `#if`, `#ifdef`, `#ifndef`, `#else`, `#endif`, `#define`, `#undef`, `#include`, `#line` — all present.
- **Stringizing (`#`) and token pasting (`##`)** in `#define` — supported.
- `#warning` and `#error` — supported.
- `__FILE__` — supported.
- **Predefined symbols:** `_OS9`, `__mc6809__`, `_BIG_END` (UG line 3648).
- "Null directive" lines (a `#` with nothing after) are not errors.

### `#include` search rules

- `#include "name"` — search includer's directory first, then standard places (UG § C.12.2, line 3137).
- `#include <name>` — search **DEFS directory on the default system drive only** (UG line 570). This is the OS-9-specific equivalent of `/usr/include`.

### `#asm` / `#endasm` (DCC extension)

UG lines 434–438. A line beginning with `#asm` switches `dcpp` into pass-through mode until `#endasm`. Caveats from the doc:

> Care should be exercised when using this directive so that the correct code section is adhered to. Normal code from the compiler is in the **psect** (code) section. If your assembly code uses the **vsect** (variable) section, be sure to put an `endsect` directive at the end to leave the state correct for later compiler-generated code.

## Run-time arithmetic error handling

UG § "Run-time arithmetic error handling," lines 521–533. Three errno values defined in `<errno.h>`:

| Value | Symbol | Meaning |
|---|---|---|
| 40 | `EFPOVR` | floating-point overflow / underflow |
| 41 | `EDIVERR` | division by zero |
| 42 | `EINTERR` | overflow on `float` → `long` conversion |

When one of these occurs, the program **sends a signal to itself** with the error number. If not caught with `signal()` / `intercept()`, the program terminates and returns the error to the parent. Other arithmetic errors are undefined.

## Standard library subset

`<stdio.h>` (always required when using stdlib), `<stdlib.h>`, `<string.h>`, `<errno.h>`, `<os9.h>`, `<sgstat.h>` are the headers used across PoCs. Detailed function list lives in `docs/reference/DCC C Compiler System Library Reference.md` ([summary stub](../sources/dcc-docs.md)).

Specific notes:
- **`<os9.h>` already defines the syscall-number macros** (`F_FORK`, `F_WAIT`, `F_SEND`, `F_SLEEP`, `F_ID`, `F_*`). Guard with `#ifndef` if redefining.
- `pflinit`/`pffinit` referencing as above.

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

The call-code constants (`I$SetStt = 0x8E`, `I$GetStt = 0x8D`, `F$Sleep = 0x0A`, `F$Time = 0x15`) are sometimes re-`#define`d in PoCs when the headers don't supply them — see PoC file headers and the `#ifndef` guard pattern.

## Object module format

DCC emits a **standard OS-9 memory module** directly — position-independent, reentrant code. UG § Ch 3 "The object code module," lines 652–738. Module header layout:

```
$00  Module header (8 bytes; Type/Lang=$11 Program+6809, Attr/Rev=$81 Reentrant+1)
$09  Execution offset (2)
$0B  Storage size (2)               — initial data+stack+param allocation
$0D  Module name (variable)
     Executable code
     String literals                ← NOT in DATA section (see below)
     Initializing Data Size (2)
     Initializing Data
     Data-Text Reference Count + Offsets   — relocations applied by cstart.r
     Data-Data Reference Count + Offsets
     CRC Check Value (3)
```

**`cstart.r` is the mainline.** Per Appendix B (line 1972), it copies initializers from the module image into the data area at startup, then walks the two reference tables fixing pointer initializers (`char *p = "..."` style) by adding the runtime base addresses of the TEXT and DATA sections.

### String literals live in the TEXT section, not DATA (UG line 713)

> No attempt should be made by a C program to alter string literals; instead, they should be copied first.

`char *p = "Hello"` puts the bytes in the read-only TEXT section. `*p = 'X';` corrupts the reentrant module image — worse than ANSI's "UB," because the module is shared across re-invocations under OS-9. **Exception** (UG line 718): `char arr[] = "Hello";` puts a copy in DATA and that copy IS modifiable. Same syntactic distinction as modern C, but the "why" on this platform is reentrancy / module-sharing rather than abstract UB.

### Module info (version / copyright)

Strings preceded by an `info` directive in assembly land in a module-info section (UG line 703). `#asm` blocks can use this to embed version strings or a copyright line in the linked module — handy for diagnostics.

### Edition number

Default 1; override with `dcc -E=n` or `dcc -e=n` (UG line 699 + Appendix B line 1981). This is the OS-9 module-edition convention, not a build counter.

## Memory layout (Level 2 — our target)

UG § Ch 3 "Memory management," lines 739–830. Level 2 detail at line 745: **Y and DP registers are zero on Level 2**, so there are no "low addresses" — the data area starts at zero in the process's address space.

```
high addresses
  parameters         ← argc/argv source area
  stack ⇓            ← S register
  ----- (gap) -----
  ⇑ free / heap      ← ibrk() raises this; sbrk() extends it
  uninitialized data ← `end` linker symbol
  initialized data   ← `edata` linker symbol
  direct page vars   ← Y, DP registers; size = however many `direct` bytes used (≥ 1)
low addresses (zero on Level 2)
```

**Y register points to the base of the data area** (UG line 801). All non-`direct` global variables are accessed Y-relative. On Level 2 with Y=0, `dco68` can replace Y-relative with absolute addressing for shorter/faster code (see Appendix D line 3668) — but this is a per-build optimization, not user-visible.

**Direct page sizing.** UG line 785: minimum 1 byte (so a pointer to a `direct` variable can never be 0/NULL). Size is determined by how many `direct` bytes the program actually uses — **not** automatically 256. Maximum 255 bytes (linker reserves 1 of 256).

**Free area** between the top of data and the bottom of the stack:
- `ibrk(n)` — request `n` more bytes of zero-initialized memory from the program's own free area.
- `sbrk(n)` — request `n` more bytes from OS-9 itself, extending the program's allocation upward (`memend`); returns -1 on failure.

### Stack-checking and the per-call overhead (UG line 778)

Each C function entry calls a system-interface routine that:
1. Reserves stack space for the function's locals + **64 bytes** for assembly callees / syscall use / arithmetic routines.
2. Records the lowest stack address granted so far.
3. If the new low would overlap data, prints `*** Stack Overflow ***` to stderr and **terminates the program**.

Per-function stack budget (UG line 825): args + locals + **4 bytes** for return address + caller's register-variable temp.

**This stack check is what `-s` suppresses** — see flag table below. Suppressing it on a 4 KB stack with deep recursion is asking for memory corruption rather than a clean abort.

### stdio buffer overhead (UG line 827)

- `stderr` is unbuffered.
- `stdin` and `stdout` are buffered.
- **256 bytes per opened-via-stdio file** (`fopen`-class) come out of the free area.
- Low-level `open`/`read`/`write` are not buffered.

So a program that `fopen`s three files needs ~768 bytes of free area beyond its data + stack just for buffers.

## Linker (`rlink`) and toolchain

DCC's compile pipeline: `dcpp` (preprocessor) → `dcc68` (compiler) → `dco68` (optimizer) → `rma` (assembler) → `rlink` (linker). Per Appendix B lines 1863–1988.

- `cstart.r` is the **mainline ROF**; the linker treats it as the entry-point module. C user code is linked as subroutines off the mainline. Other ROFs cannot be marked mainline.
- `clib.l` (stdlib + math + system interface) and `cstart.r` must live in the `LIB` directory of the default system drive (UG line 566).
- **Predefined linker symbols:** `etext` (end of code), `edata` (end of initialized data), `end` (end of uninitialized data) — UG line 577.
- **External-name significance: 8 characters** on 6809 (UG line 2013).
- **All input ROFs are linked unconditionally** — but `-l=` library files are searched only for unresolved references (Appendix B line 1980). Standard "explicit object vs library" distinction.
- `dco68` patterns are external `*.patterns` files (Appendix D line 3666).
- **`dcpp` derives the psect name** from the last path element of the input + `_c`: `/d0/myprog.c` → psect `myprog_c` (Appendix B line 1913).

## Compile flags — full table

From UG § Ch 2 "Compiler option flags" (lines 616–633) and Appendix B (lines 1863–1988). **Flags can be combined like `-ro`** (UG line 618). The Ch 2 table and Appendix B table differ in coverage; Appendix B is the authoritative superset.

| Flag | Meaning | Source |
|------|---------|--------|
| `-2` | Optimize for NitrOS-9 Level 2. **"Beware, attempting to run a program compiled using this option is extremely likely to fail, and may even crash the computer!"** (Appendix B line 1883). See [lessons-learned.md](../implementation/lessons-learned.md). | App. B |
| `-a` | Stop after compile; leave assembly in `.a` file | Ch 2 / App. B |
| `-b=path` | Use alternate `cstart` file (mainline ROF) | App. B 1885 |
| `-c` | Embed C source as comments in generated assembly (debugging) | Ch 2 630 / App. B |
| `-d<NAME>[=<value>]` | `#define <NAME> <value>` (or `1` if no value) | Ch 2 632 / App. B |
| `-e=<n>` / `-E=<n>` | Edition number for module header | Ch 2 623 / App. B 1889 |
| `-f=path` | Override output file path; module name = last path element | Ch 2 629 / App. B |
| `-l=path` | Add library to linker search **before** the standard library | Ch 2 628 / App. B |
| `-lg` | Link with `cgfx.l` — **Tandy Color Computer 3 graphics library** | App. B 1892 |
| `-ll` | Link with `lexlib.l` (Lex helper) | App. B 1893 |
| `-ls` | Link with `sys.l` — **definitions for OS-9 system calls, constants, and `I$GetStt`/`I$SetStt` codes** | App. B 1894 |
| `-M` | Request linkage map from linker | App. B 1895 |
| `-M=size` | (`rlink`-level) Linker data+stack+param allocation, in pages or `<n>k` | Ch 2 627 / App. B 1983 |
| `-m=size` | (`dcc`-level) Additional memory for linker, pages or `<n>k`. **Linker ignores < 256 B** (UG line 819) | App. B 1896 |
| `-n=name` | Output module name (overrides `-f=` default) | App. B 1906 |
| `-O` | Stop after optimizing; leave optimized assembly in `.a` file | App. B 1897 |
| `-o` | **Inhibit** the assembly-code optimizer (use during error-checking-only builds) | Ch 2 624 / App. B 1898 |
| `-P` | Profiler **with debug variant** (links `dbg.l` debugging library) | App. B 1900 |
| `-p` | Add profiler calls (`_prof`) — function call counts printed at exit | Ch 2 625 / App. B 1899 |
| `-r` | Stop before linking; leave `.r` files | Ch 2 626 / App. B |
| `-S` | Request symbol table from linker | App. B 1902 |
| `-s` | **Suppress generation of stack-checking code.** "Should only be used with great care when the application is extremely time-critical and when the use of the stack by compiler-generated code is fully understood" (Ch 2 631 + App. B 1903 + dcc68 1935). **NOT a speed-vs-size flag.** | Ch 2 631 / App. B |
| `-T[=path]` | Set/disable temp directory for intermediate files | App. B 1904 |
| `-q` | Quiet mode; redirect stderr to `c.errors` | App. B 1907 |

Recommended starter from GDD §23.8:

```
dcc mule.c map.c players.c auction.c ai.c render.c sound.c -m=32k -o=mule
```

(Note: the `-o=mule` here predates the Appendix B clarification — `-o` lowercase actually inhibits the optimizer. The intended flag is presumably `-f=/dd/cmds/mule` for the output path. Cross-check before adopting.)

## Flags seen in PoCs

| Flag | Actual meaning (per Appendix B) | Used in |
|------|---------------------------------|---------|
| `-s` | **Suppresses stack-checking code** — NOT speed optimization. PoCs running under this flag have no stack-overflow detection. See [lessons-learned.md](../implementation/lessons-learned.md). | Multiple PoCs |
| `-m=4k`, `-m=8k`, `-m=24k`, `-m=32k` | Linker allocates that much **additional memory** beyond program data+strings (covers stack, parameter area, libc buffers, free area for `ibrk`/`sbrk`). | Per-PoC tuning |
| `-f=/dd/cmds/<name>` | Output module path; module name = last path element. | All PoCs |

Example from [poc_cvdg16.c](../sources/poc-sources.md):

```
dcc poc_cvdg16.c -s -m=4k -f=/dd/cmds/poccvd16
```

## Memory layout (high level)

UG Ch 3 § "Memory management," lines 739–830. Process address space contains code (psect), initialized data (vsect), the direct-page slot, and the stack/parameter area allocated by the linker. Cross-reference [memory.md](memory.md) for process-level layout under NitrOS-9 Level 2.

## Sources

- `docs/reference/DCC C Compiler System Users Guide.md` — Ch 2 (lines 387–649), Ch 3 (lines 650–830), Appendix B (lines 1863–1988), Appendix C (lines 1989–3617), Appendix D (lines 3618–3677). See [../sources/dcc-docs.md](../sources/dcc-docs.md) for section-to-line map.
- `docs/reference/DCC C Compiler System Library Reference.md` ([summary stub](../sources/dcc-docs.md)).
- `AGENTS.md` DCC lessons.
- `docs/design/MULE_GDD.md` §23.8.
