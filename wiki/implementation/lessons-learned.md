# Lessons Learned

Observed-fact findings from PoC work. Mirrors AGENTS.md §66+ with links to the wiki pages that act on each finding. Do **not** remove items here without replacing with a clear "superseded by X" note.

## DCC / language ([dcc.md](../platform/dcc.md))

- **K&R dialect only.** Declarations at start of block; no `void` prototypes; no modern C syntax. Keep external symbol names short and distinct to reduce linker/name-collision risk.
- **16-bit `int`.** Plan data sizes accordingly.
- **Linker collides on long shared name prefixes.** `phase_land_grant` and `phase_land_auction` produced `multiple definition` at link time — even with full names that differ in characters 11+. Mitigation: keep the distinct portion of an external name within the first ~6 chars, and mark file-local helpers `static`. Observed 2026-04-25 building [main.c](../../src/c/main.c).
- **NitrOS-9 ABI is size-based, not type-based.** Both Basic09 and DCC C pass parameters by *size in bytes*, with no automatic widening of `char` (1 B) to `int` (2 B) at varargs / unprototyped call sites. Passing `unsigned char` directly to `printf("%d", ...)` makes printf read 2 bytes per `%d`, get stack garbage in the high byte, format an out-of-range integer, and the terminal interprets the resulting high-bit / control bytes as block-graphic glyphs ("graphic bars down the screen"). **Mitigation:** prefer `int` for any value that crosses a function boundary; if a struct field must be `char` for size, widen with `(int)` at every call site. Confirmed by user 2026-04-25 ("known aspect of coding for NitrOS-9 using Basic09").
- **`direct` storage class — status uncertain.** Initial crash of `pp` was provisionally blamed on `direct GameState g_state` (hypothesis: collision with libc's direct-page slots `_flacc`, `errno`). Removing `direct` did **not** fix the crash — the real cause was the printf-promotion bug above. The `direct` mechanism therefore remains untested rather than refuted; treat as a future PoC target. No existing PoC in `src/c/` uses it.
- **`<os9.h>` already defines the syscall numbers.** `F_ID`, `F_FORK`, `F_WAIT`, `F_SEND`, `F_SLEEP` (and likely the rest) ship in DCC's `os9.h`. Plain `#define` redeclaration produces `redefined macro` warnings — guard with `#ifndef` per the [poc_vsync.c](../../src/c/poc_vsync.c) pattern. Observed 2026-04-25 building [poc_ipc.c](../../src/c/poc_ipc.c).
- **`/term` must be opened mode 3 (read/write) for `SS.Tone`.** Opening write-only (mode 1) silently fails — the `I_SETSTT` call returns no error, but no sound is produced. The working [poc_sound.c:165](../../src/c/poc_sound.c) uses `open("/term", 3)`. Confirmed 2026-04-25 — `pocsnd` produced no sound until the child's `open("/term", 1)` was changed to mode 3.
- **`SS.Tone` is interruptible by signals.** A blocking `I_SETSTT(SS.Tone)` call returns early when a signal is delivered — and when it does, the tone aborts inaudibly. **Implication for the multi-process architecture:** any process that calls `SS.Tone` cannot also use `F$Send` as its wake mechanism — the producer's signals will silently abort tones almost immediately. Phase-3 sound child uses a short `F$Sleep` (2 ticks ≈ 33 ms) poll instead. Observed 2026-04-25 building [poc_sndc.c](../../src/c/poc_sndc.c): all four queued tones were silent under signal-wake; switching to poll-wake produced clean playback.
- **`#asm ldy 6,s` from a Basic09-callable module corrupts global access when the same source is linked into a C program.** When porting C source written to be invoked as a standalone module from Basic09 (e.g. stocks-and-bonds `SLPICPT.c`), watch for an `ldy <offset>,s` preamble — that's there because Basic09's `RUN` directive doesn't set up the Y register the way DCC's C ABI expects, so the function re-syncs it from a stacked argument. When the same source is linked into a regular C program, Y already holds the data-segment pointer for global access; the preamble overwrites it with whatever happens to be at the named stack offset (typically a constant int from the caller), and every subsequent global-variable access in the call chain reads wild memory. Symptom: caller hangs after the function returns. Observed 2026-04-25 — `pocipc` hung after "parent pid=3" because the `ldy 6,s` line in `slpicpt.c` clobbered Y with the literal `1` from the caller's third argument. **Mitigation:** when porting a Basic09-callable C module to a C-only build, remove any leading `#asm ld<reg> N,s` preamble. The b09 caller is the only context where it makes sense.
- **Module crashes can survive past the source change that caused them.** While debugging the phase-1 skeleton we observed several non-deterministic crashes (graphic bars, vertical stripes, full reset) that disappeared after stripping `main.c` to bare minimum, then *did not reappear* when we restored each suspect change one at a time (PRs #15 / #17 / #18). The same code that crashed previously ran clean afterwards. Modules on EOU are file-based (overwritten by `dcc -f=...`) — there is no in-kernel module-cache to flush — so the persistence had to come from elsewhere: a partially-written disk-image module from an interrupted build, a `buildc` that ran with stale included sources, or platform state seeded by an earlier broken run. **Mitigation:** when a NitrOS-9 module crashes, before bisecting the source, rebuild from clean (`buildc`, not `patchc`) and re-deploy the disk image fresh. Recorded 2026-04-25.

### From User's Guide ingest 2026-04-25 — non-obvious quirks

- **External-name significance is exactly 8 characters on 6809.** DCC User's Guide § C.2.2 (line 2013) documents: "6809 — 8 characters, 2 cases." This **refines** the prior empirical observation above (recorded as "first ~6 chars"). The `phase_land_grant` / `phase_land_auction` collision was real: both share `phase_la` for the first 8 characters. **Update the rule:** keep the distinct portion of any external (non-`static`) name within the **first 8 characters**. Applies to function names and any non-`static` global. File-local helpers should still be `static` to bypass the limit entirely.
- **Spec-vs-implementation contradiction on call-site type promotion.** UG § C.7.1 (line 2216) says: "Any actual arguments of type `char` or `short` are converted to `int`. Any of type `float` are converted to `double`." UG § C.10.1 (line 3039) repeats this for formal-parameter adjustment. **But** our printf-graphic-bars finding above shows that `unsigned char` passed to `printf("%d", …)` is *not* promoted — only 1 byte is pushed and `%d` reads 2 bytes. **Tentative resolution:** the spec-claimed promotion happens only when there is a formal parameter to drive it. With varargs (`…`) and unprototyped calls, no formal parameter exists, the size-not-type ABI dominates, and `char` is pushed as 1 byte. **Practical rule:** treat call-site promotion as unreliable and always cast `(int)` at the call for variadic / unprototyped calls. Recorded 2026-04-25.
- **`\n` in DCC string literals is `\r` (0x0D), not LF (0x0A).** UG line 400 — OS-9 uses CR for end-of-line. Programs that treat `\n` as LF (e.g. when generating byte streams to send over a serial link or to a non-OS-9 file format) will silently emit CR. Use the DCC-specific `\l` escape (UG line 445) when an actual LF byte is required.
- **Auto aggregates and unions cannot be initialized.** UG line 2683: "It is not permitted to initialize unions or automatic aggregates." `struct Foo f = { 1, 2 };` at function scope is a hard error in DCC. Either (a) make it `static`, (b) zero with `memset` and assign field-by-field, or (c) initialize at the call site. Modern C makes auto-struct initialization look obvious; DCC rejects it.
- **One `register` variable per function, silently downgraded if violated.** UG lines 493–497: only `int` / `unsigned int` / pointer types qualify; only the first `register` declaration takes effect; all others become `auto` with no warning. The U register is committed to the chosen register variable. Don't sprinkle `register` across many vars expecting any benefit beyond the first.
- **`pflinit`/`pffinit` referencing dance for `printf` long/float formatting.** UG line 519. By default `printf` does NOT include `%ld` or `%f` formatters — to save memory. Without an `extern pflinit();` (long) or `extern pffinit();` (float) reference somewhere reachable, `printf("%ld", x)` will silently misformat. This is *not* a stdlib link error — it's a quiet failure. Major source of "why is my number garbage" surprises.
- **String literals are NOT pooled** (UG line 2073: "All strings, even when written identically, are distinct"). Don't compare string-literal pointers expecting equality.
- **Floating arithmetic always promotes to `double` first** (UG line 2147). Even `float a, b; a = a + b;` runs through double-precision; performance cost as if you'd used `double`. UG line 539 already warns: don't use `long`/`float`/`double` where `int` will do — they cost ~4× in code size and time.
- **Float / double in DCC are NOT IEEE 754.** UG lines 481–491 — DCC uses sign+magnitude with biased exponent and 56-bit (`double`) or 24-bit (`float`) mantissa. Binary float values exchanged with anything that expects IEEE 754 will be misinterpreted.
- **No `void` and no `enum` in the keyword list** (UG § C.2.3, lines 2017–2030). Use empty `()` for "no args"; use `char *` for generic pointers; use `#define` constants for enumerations. Already on the AGENTS.md `void` list; `enum` absence is added here.
- **Grammar in Appendix C does NOT list `direct`** (UG § C.18.2, line 3458) — it's a Chapter 2 / DCC-specific extension. If you cross-reference the K&R reference manual section to validate syntax, you will not find `direct` there. Don't conclude it's unsupported — Chapter 2 lines 404–432 are authoritative.
- **`#asm` blocks: `vsect` users must `endsect` before `#endasm`** (UG line 438), or the compiler's later code generation will be in the wrong section. Easy to miss; documented exactly once.
- **DCC's `<` `>` includes search the DEFS directory only** (UG line 570) — not a chain of include paths. Project-private headers must use `"…"` form to be found.

### From User's Guide ingest 2026-04-25 (Tier 2 — ABI / module / toolchain)

- **`-s` does NOT mean "optimize for speed."** UG § Ch 2 line 631 + Appendix B lines 1903 / 1935: `-s` **suppresses generation of stack-checking code**. Prior wiki ([dcc.md](../platform/dcc.md), [lessons-learned.md](../implementation/lessons-learned.md), GDD §23.8) and AGENTS.md historically described it as a speed flag. **It is not.** PoCs that pass `-s` have no `*** Stack Overflow ***` runtime check — a stack collision into the data area becomes silent memory corruption rather than a clean abort. On 4 KB stacks, this is a real risk in any function with deep call chains, recursion, or large auto buffers. **Action:** when porting/extending PoCs, default to dropping `-s`; only re-add it after the program is debugged AND stack high-water has been measured. Recorded 2026-04-25.
- **`-2` (Level 2 optimization) is documented as dangerous** for our exact target. Appendix B line 1883: "Beware, attempting to run a program compiled using this option is extremely likely to fail, and may even crash the computer!" We are running on NitrOS-9 EOU which IS Level 2, and `dco68` already has Level-2 patterns it applies on its own (Appendix D line 3668 — absolute addressing because the data area starts at zero). **Don't use `-2` even though our target is Level 2.** The optimizer's built-in Level-2 patterns handle what's safely automatable.
- **`-ls` exists and links `sys.l` with OS-9 syscall constants, `I$GetStt`/`I$SetStt` codes, and SS_* constants** (Appendix B line 1894). Our PoCs currently `#define` these manually (`I$SetStt = 0x8E`, `SS_TONE = 0x98`, etc.) and guard with `#ifndef` against `<os9.h>`. `-ls` is the documented alternative. **Worth a small PoC to verify what symbols `sys.l` actually exposes** — could simplify every existing PoC's syscall-number boilerplate.
- **`-lg` links `cgfx.l` — a Tandy Color Computer 3 graphics library** (Appendix B line 1892). Existence not previously known to the project. Its scope and overlap with CoVDG/CoWin is unknown. **Worth a discovery query** before designing render-side code: if it provides primitives that match what CoWin already does, we may be reinventing wheels.
- **String literals are in the module's TEXT section, not DATA** (UG line 713). Modifying `*p` after `char *p = "Hello"` corrupts a section of the executing memory module — which under NitrOS-9 is reentrant and may be shared across re-invocations of the same program. Worse than ANSI's "UB"; this can affect other instances of the program. `char arr[] = "Hello"` puts a modifiable copy in DATA — use that form when mutation is needed (UG line 718).
- **Each function entry costs +64 bytes of stack reservation beyond locals** (UG line 778), reserved for assembly callees / syscall use / arithmetic helpers. Plus 4 bytes for return address + caller's register-variable save (UG line 825). When budgeting `-m=`, plan call-depth × (locals + 4 + 64) at minimum, plus the parameter area, plus 256 B per `fopen`'d file (UG line 827; not for low-level `open`/`read`/`write`).
- **`stderr` is unbuffered; `stdin`/`stdout` get 256-byte buffers each** (UG line 827). Programs that route diagnostics through `stderr` get immediate flush; programs that mix `stdout` with `printf` must `fflush` to see output before a crash.
- **The default `-m=` overhead is +1 KB above program data + strings** (UG line 807). Linker silently ignores `-m=` requests below 256 bytes (UG line 819). So `-m=128` is a no-op, not an error.
- **`ibrk()` / `sbrk()` exist** (UG line 803) for runtime memory request — `ibrk(n)` from the program's own free area (zero-initialized), `sbrk(n)` from OS-9 itself (returns -1 on failure). Worth knowing for dynamic-allocation sketches; neither is a `malloc` replacement on its own.
- **`dcpp` derives the psect name from the input filename + `_c`** (Appendix B line 1913): `/d0/myprog.c` → psect `myprog_c`. Two source files with the same basename in different directories will produce identically-named psects — potential link-time clash distinct from the 8-char identifier rule above. Keep `.c` filenames distinct across directories that feed the same link.
- **`info` directive (via `#asm`) embeds version/copyright strings in the OS-9 module info section** (UG line 703). Lightweight way to stamp build identity into the module without a runtime cost.

## [CoWin](../platform/cowin.md) GET/PUT sprite movement

- `GetBlk`/`PutBlk` **are fast enough** for small sprite tests when the buffer contains the **union of restored background and new sprite**.
- **One 40×40 union buffer per movement direction** reduced flicker better than separate background restore + sprite put.
- Use **byte-aligned X coordinates** where possible; 8-pixel steps succeeded in testing.
- **CoWin command buffering does NOT provide true page flipping or guaranteed VBlank synchronization.**

## [Timing](../platform/timing.md)

- **VRN `/nil` `SS.FSet`** provides a documented 1/60s timing gate — good for measurement and frame pacing, **not** for tear-free hardware-tight vsync.
- **`F$Time` returns only whole seconds** — FPS/timing output is coarse. If a test reports unexpectedly slow timing, treat that as observed fact and run a discriminating test before assuming a cause.

## [CoVDG](../platform/covdg.md) screens

- Call `SS.AScrn`, `SS.DScrn`, `SS.FScrn` with `I$SetStt` on a **VTIO/CoVDG VDG path**, not a CoWin window.
- `/verm` has been observed to accept `SS.AScrn` where `term_vdg` and `TERM_VDG` did not.
- `SS.DScrn` only displays if the CoVDG path is the **current interactive device**.
- Send CoVDG `Select` (`ESC $21`) to the opened VDG path **before** expecting `SS.DScrn` output to become visible.
- Screen 0 should be **displayed before freeing** allocated high-resolution screens.

## CoVDG memory

- **320×192×16 CoVDG screen type 4** requires 32 KB; a **second** 32K allocation failed with err #207 in testing.
- **160×192×16 CoVDG screen type 2** requires 16 KB and is the **viable page-flip target**.

## [Sound](../platform/sound.md)

- **`SS.Tone` is BLOCKING** — process pauses for the full tone duration.
- Production architecture **requires a dedicated sound process** reading `{freq, dur, amp}` tuples from a pipe.
- Frequency is a **relative counter 0–4095**, not Hz. Widest pitch variation at the high range; PoC intentionally uses high values.

## Workflow

- Leave repository files as source of truth; the deploy/build workflow owns `disks/ppsrc.dsk`.
- Prefer **reference-document-confirmed interfaces** before changing PoC code. Primary sources: NitrOS-9 Technical Reference, Level 2 Windowing System Manual, DCC references.

## Sources

- `AGENTS.md` §66–94
- PoC file headers in `src/c/`
