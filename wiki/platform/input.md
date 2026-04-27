# Input

## Joystick — `SS.Joy`

`I$GetStt` code `0x13` on a terminal/window path. Reads right (0) or left (1) joystick.

From GDD §23.4:

```c
#include <sgstat.h>

void read_joystick(unsigned char port, unsigned char *xval,
                   unsigned char *yval, unsigned char *button)
{
    struct registers r;
    r.rg_a = g_term_path;
    r.rg_b = 0x13;           /* SS.Joy */
    r.rg_x = port;           /* 0=right, 1=left */
    _os9(0x8D, &r);          /* I$GetStt */
    *xval   = r.rg_x & 0xFF;
    *yval   = r.rg_y & 0xFF;
    *button = r.rg_a;
}
```

## Keyboard

- `I$ReadLn` for blocking line reads (config, text prompts).
- `SS.KySns` GetStat (function $27) for **real-time key sensing**. A returns 8-bit pattern: `bit0 SHIFT  bit1 CTRL  bit2 ALT  bit3 UP  bit4 DOWN  bit5 LEFT  bit6 RIGHT  bit7 SPACE`. Confirmed live on EOU 2026-04-26 via `poc_input`.
- `SS.KySns` SetStat (also $27, X≠0 enables key-sense-only mode, X=0 restores) suppresses the 8 special keys (arrows, space, modifiers) from SCF. **Letters/digits still flow through SCF** even in key-sense mode (confirmed live 2026-04-26), so [`input.c`](../../src/c/input.c) also drains stdin each `inp_poll` and at `inp_shut` via `SS.Ready` ($01) + `read` until E$NotRdy. Without the drain, anything typed during a phase spills to the shell command line on exit.

In key-sense-only mode the only way to detect any key press is `SS.KySns` GetStat — `I$Read` becomes inert. That is fine for the in-game phases (management, auctions); text prompts must restore normal mode first.

## Input mapping (GDD §20)

- Four joysticks for four players when available.
- **Spacebar pauses/unpauses during management phase.**
- **No pause during auction phases.**

## Simultaneous-press window

For Land Grant and auction synchronization: ~100 ms (`LAND_GRANT_TIE_WINDOW_MS = 100`, Planet M.U.L.E. clarification). If two players press within this window on the same plot, poorer player wins (tie → lower player index).

## Sources

- `docs/design/MULE_GDD.md` §20, §23.4
- `docs/reference/DCC C Compiler System Library Reference.md` (for `_os9` calling convention — stub)
