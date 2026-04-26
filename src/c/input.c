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
 * API (host declares with `extern`):
 *   inp_init()         -> 0 ok, non-zero err
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

static unsigned char cur;
static unsigned char prev;

int inp_init()
{
    cur = 0;
    prev = 0;
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
