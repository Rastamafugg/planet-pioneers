/***********************************************************************
 * input.c  -  Polled keyboard input via SS.KySns (phase 5).
 *
 * Single-process, no fork. SS.KySns is non-blocking, so the logic
 * process polls it directly each frame. State is kept in module-
 * static `cur` / `prev` so press/release edges can be derived.
 *
 * Tech Ref §SS.KySns (function $27): A=path, B=$27 -> A returns a
 * bit pattern at the last hardware scan:
 *   bit0 SHIFT  bit1 CTRL  bit2 ALT
 *   bit3 UP     bit4 DOWN  bit5 LEFT  bit6 RIGHT  bit7 SPACE
 * Confirmed live on EOU via poc_input (PR #39, #40).
 *
 * Public symbol names use an `inp_` prefix; DCC's external-name
 * significance is 8 characters (lessons-learned).
 *
 * inp_init flips the keyboard into key-sense-only mode via SS.KySns
 * SetStat (X != 0): VTIO stops forwarding keypresses to the SCF
 * buffer, so arrow/space keystrokes don't show up on the terminal
 * after the program exits. inp_shut restores normal mode (X = 0).
 * Hosts MUST call inp_shut before exit on every path.
 *
 * API (host declares with `extern`):
 *   inp_init()         -> 0 ok, non-zero err  (enables key-sense mode)
 *   inp_shut()         -> 0 ok                (restores normal kbd mode)
 *   inp_poll()         -> 0 ok, non-zero err  (call once per frame)
 *   inp_held(mask)     -> nonzero if any of mask is currently down
 *   inp_pres(mask)     -> nonzero if any of mask edge-pressed since last poll
 *   inp_rele(mask)     -> nonzero if any of mask edge-released since last poll
 *
 * Bit masks (bit pattern from Tech Ref):
 *   K_SHIFT 0x01 K_CTRL 0x02 K_ALT 0x04
 *   K_UP    0x08 K_DOWN 0x10 K_LEFT 0x20 K_RIGHT 0x40 K_SPACE 0x80
 *
 * Compile (linked into hosts): `dcc <host>.c input.c ...`
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef SS_KYSNS
#define SS_KYSNS 0x27
#endif
#ifndef I_GETSTT
#define I_GETSTT 0x8D
#endif
#ifndef I_SETSTT
#define I_SETSTT 0x8E
#endif
#ifndef SS_READY
#define SS_READY 0x01
#endif
#ifndef SS_OPT
#define SS_OPT   0x00
#endif

/* Offset of PD.EKO inside the 32-byte PD.OPT buffer.
 * Tech Ref: PD.OPT starts at PD offset $20; PD.EKO sits at PD $24
 * (line 1896) -> buffer offset $24 - $20 = $04. */
#define PD_EKO_OFF 0x04

static unsigned char cur;
static unsigned char prev;
static unsigned char saved_opt[32];

/* Discard any letters/digits sitting in stdin's SCF buffer. SS.KySns
 * key-sense-only mode suppresses the 8 special keys but printable
 * keys still flow through SCF; without draining they spill onto the
 * shell command line after pioneer exits. */
static drain()
{
    struct registers r;
    char ch;

    for (;;) {
        r.rg_a = 0;
        r.rg_b = SS_READY;
        if (_os9(I_GETSTT, &r)) return;  /* not ready -> empty */
        if (read(0, &ch, 1) <= 0) return;
    }
}

int inp_init()
{
    struct registers r;
    unsigned char work_opt[32];
    int i;

    cur = 0;
    prev = 0;

    /* Snapshot stdin's PD.OPT, then write a copy with PD.EKO=0 to
     * disable terminal echo for the duration of the run. */
    r.rg_a = 0;
    r.rg_b = SS_OPT;
    r.rg_x = (unsigned)saved_opt;
    if (_os9(I_GETSTT, &r)) return -1;
    for (i = 0; i < 32; i++) work_opt[i] = saved_opt[i];
    work_opt[PD_EKO_OFF] = 0;
    r.rg_a = 0;
    r.rg_b = SS_OPT;
    r.rg_x = (unsigned)work_opt;
    if (_os9(I_SETSTT, &r)) return -1;

    /* Enable key-sense-only mode for the 8 special keys. */
    r.rg_a = 0;
    r.rg_b = SS_KYSNS;
    r.rg_x = 1;
    if (_os9(I_SETSTT, &r)) return -1;
    return 0;
}

int inp_shut()
{
    struct registers r;

    drain();

    /* Restore normal keyboard mode. */
    r.rg_a = 0;
    r.rg_b = SS_KYSNS;
    r.rg_x = 0;
    _os9(I_SETSTT, &r);

    /* Restore the original PD.OPT (re-enables echo). */
    r.rg_a = 0;
    r.rg_b = SS_OPT;
    r.rg_x = (unsigned)saved_opt;
    _os9(I_SETSTT, &r);
    return 0;
}

int inp_poll()
{
    struct registers r;

    prev = cur;
    r.rg_a = 0;
    r.rg_b = SS_KYSNS;
    if (_os9(I_GETSTT, &r)) return -1;
    cur = r.rg_a & 0xff;
    drain();  /* discard any printable keys typed since last poll */
    return 0;
}

int inp_held(mask)
unsigned char mask;
{
    return (cur & mask) != 0;
}

int inp_pres(mask)
unsigned char mask;
{
    return (cur & ~prev & mask) != 0;
}

int inp_rele(mask)
unsigned char mask;
{
    return (~cur & prev & mask) != 0;
}
