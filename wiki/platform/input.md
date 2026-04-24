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
- `SS.KySns` GetStat for **real-time key sensing** — shift, arrow keys, etc. Needed for responsive in-game keyboard controls.

## Input mapping (GDD §20)

- Four joysticks for four players when available.
- **Spacebar pauses/unpauses during management phase.**
- **No pause during auction phases.**

## Simultaneous-press window

For Land Grant and auction synchronization: ~100 ms (`LAND_GRANT_TIE_WINDOW_MS = 100`, Planet M.U.L.E. clarification). If two players press within this window on the same plot, poorer player wins (tie → lower player index).

## Sources

- `docs/design/MULE_GDD.md` §20, §23.4
- `docs/reference/DCC C Compiler System Library Reference.md` (for `_os9` calling convention — stub)
