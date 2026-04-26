/***********************************************************************
 * render.c  -  Parent-side render API (phase 4).
 *
 * Forks a `pocrndc` child that owns the CoVDG path, two 16K screens,
 * tile/sprite drawing, and the page-flip. The parent's logic process
 * enqueues 8-byte RenderCmd intents into a 64-entry SPSC ring in
 * shared memory; the child polls (F$Sleep 1 tick) and drains.
 *
 * Mirrors phase-3 sound.c. No signals — wakeup is poll on both sides.
 *
 * RenderQueue layout MUST match poc_rndc.c.
 *
 * Public symbol names use a `ren_` prefix because DCC's external-name
 * significance is exactly 8 characters (lessons-learned 2026-04-25):
 * `render_present`/`render_palette` collide on `render_p`, and
 * `render_sprite`/`render_shutdown` collide on `render_s`.
 *
 * API:
 *   ren_init()                                         -> 0 ok, !=0 err
 *   ren_clr(color)                                     -> 0 / -1 (full)
 *   ren_tile(kind, col, row)                           -> 0 / -1
 *   ren_dmap()                                         -> 0 / -1
 *   ren_spr(slot, x, y, frame, dir, mule)              -> 0 / -1
 *   ren_pal(idx, rgb)                                  -> 0 / -1
 *   ren_pres()                                         -> 0 / -1
 *   ren_flush()                                        -> 0 (blocks)
 *   ren_shut()                                         -> 0
 *
 * Compile (linked into hosts): listed in `dcc <host>.c render.c ...`
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

#ifndef F_FORK
#define F_FORK   0x03
#endif
#ifndef F_WAIT
#define F_WAIT   0x04
#endif
#ifndef F_SEND
#define F_SEND   0x08
#endif
#ifndef F_SLEEP
#define F_SLEEP  0x0A
#endif
#ifndef F_ID
#define F_ID     0x0C
#endif
#ifndef F_ALLRAM
#define F_ALLRAM 0x39
#endif
#ifndef F_MAPBLK
#define F_MAPBLK 0x3D
#endif
#ifndef F_CLRBLK
#define F_CLRBLK 0x38
#endif
#ifndef F_DELRAM
#define F_DELRAM 0x37
#endif

#define RENDER_MAGIC      0x5244
#define RENDER_QUEUE_SIZE 64
#define RENDER_QUEUE_MASK (RENDER_QUEUE_SIZE - 1)

/* Signal numbers reserved per [wiki/platform/ipc.md].
 * 130 used to be SIG_RENDER_ACK (reserved) — promoted to actual use
 * here as the child→parent "frame done" wake. 133 added for the
 * parent→child "drain queue" wake. */
#define SIG_RENDER_DONE 130
#define SIG_RENDER_WAKE 133

#define R_OP_CLEAR    1
#define R_OP_TILE     2
#define R_OP_SPRITE   3
#define R_OP_PRESENT  4
#define R_OP_PALETTE  5
#define R_OP_DRAWMAP  6

typedef struct {
    unsigned char op;
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned int  x;
    unsigned int  y;
} RenderCmd;

typedef struct {
    unsigned int magic;
    unsigned int head;
    unsigned int tail;
    unsigned int quit;
    unsigned int ready;
    unsigned int parent_pid;     /* set by parent before fork; child reads */
    RenderCmd    entries[RENDER_QUEUE_SIZE];
} RenderQueue;

static RenderQueue *g_rq;
static int          g_rblock;
static int          g_rpid;

/* Signal intercept — single-buffer, shared with any other future
 * subsystem that wants parent-side intercept. Currently only render
 * uses it. */
extern int intercept();
static int g_lastsig;

static int rd_sigtrap(sig)
int sig;
{
    g_lastsig = sig;
    return 0;
}

static int rd_enq(op, a, b, c, x, y)
int op, a, b, c, x, y;
{
    int next;
    RenderCmd *e;

    if (g_rq == 0) return -2;
    next = (g_rq->head + 1) & RENDER_QUEUE_MASK;
    if (next == g_rq->tail) return -1;
    e = &g_rq->entries[g_rq->head];
    e->op = (unsigned char)op;
    e->a  = (unsigned char)a;
    e->b  = (unsigned char)b;
    e->c  = (unsigned char)c;
    e->x  = (unsigned)x;
    e->y  = (unsigned)y;
    g_rq->head = next;
    return 0;
}

int ren_init()
{
    struct registers r;
    char  name[8];
    char  param[16];
    int   plen, waited;

    g_rq = 0;
    g_rblock = -1;
    g_rpid = -1;

    r.rg_b = 1;
    if (_os9(F_ALLRAM, &r)) {
        printf("render: F$AllRAM err #%d\n", r.rg_b & 0xff);
        return 1;
    }
    g_rblock = ((r.rg_a & 0xff) << 8) | (r.rg_b & 0xff);

    r.rg_b = 1;
    r.rg_x = g_rblock;
    if (_os9(F_MAPBLK, &r)) {
        printf("render: F$MapBlk err #%d\n", r.rg_b & 0xff);
        return 2;
    }
    g_rq = (RenderQueue *)r.rg_u;

    g_rq->magic = RENDER_MAGIC;
    g_rq->head  = 0;
    g_rq->tail  = 0;
    g_rq->quit  = 0;
    g_rq->ready = 0;

    /* Install signal intercept BEFORE the fork so any DONE the child
     * sends (even racing) lands in our trap, not as a default-action
     * process kill. */
    g_lastsig = 0;
    intercept(rd_sigtrap);

    /* Record our PID so the child knows where to F$Send DONE. */
    if (_os9(F_ID, &r) == 0) {
        g_rq->parent_pid = r.rg_a & 0xff;
    } else {
        g_rq->parent_pid = 0;
    }

    sprintf(param, "%d", g_rblock);
    plen = strlen(param);
    param[plen++] = '\r';

    name[0] = 'p'; name[1] = 'o'; name[2] = 'c';
    name[3] = 'r'; name[4] = 'n'; name[5] = 'd';
    name[6] = 'c' | 0x80;

    r.rg_a = 0;
    r.rg_b = 7;
    r.rg_x = (unsigned)name;
    r.rg_y = plen;
    r.rg_u = (unsigned)param;
    if (_os9(F_FORK, &r)) {
        printf("render: F$Fork err #%d\n", r.rg_b & 0xff);
        return 3;
    }
    g_rpid = r.rg_a & 0xff;

    /* Wait for child to allocate its screens and signal ready. Poll
     * F$Sleep 5 ticks; bail after ~30 sec. (Was 5 sec — too tight on
     * emulators paced below real CoCo speed; child does software rects
     * during init which can plausibly run >1 sec.) */
    for (waited = 0; waited < 360; waited++) {
        if (g_rq->ready) return 0;
        r.rg_x = 5;
        _os9(F_SLEEP, &r);
    }
    printf("render: child not ready after 30s\n");
    return 4;
}

int ren_clr(color)
int color;
{
    return rd_enq(R_OP_CLEAR, color, 0, 0, 0, 0);
}

int ren_tile(kind, col, row)
int kind, col, row;
{
    return rd_enq(R_OP_TILE, kind, col, row, 0, 0);
}

int ren_spr(slot, x, y, frame, dir, mule)
int slot, x, y, frame, dir, mule;
{
    int packed;
    /* c byte: dir in low 4 bits, mule flag in bit 4. */
    packed = (dir & 0x0f) | ((mule & 1) << 4);
    return rd_enq(R_OP_SPRITE, slot, frame, packed, x, y);
}

int ren_dmap()
{
    return rd_enq(R_OP_DRAWMAP, 0, 0, 0, 0, 0);
}

int ren_pal(idx, rgb)
int idx, rgb;
{
    return rd_enq(R_OP_PALETTE, idx, rgb, 0, 0, 0);
}

int ren_pres()
{
    return rd_enq(R_OP_PRESENT, 0, 0, 0, 0, 0);
}

int ren_flush()
{
    struct registers r;

    if (g_rq == 0) return -2;

    /* If there's pending work, kick the child so it doesn't sleep
     * past it. Then F$Sleep(0) until the child sends DONE. */
    if (g_rq->tail != g_rq->head) {
        r.rg_a = (char)g_rpid;
        r.rg_b = (char)SIG_RENDER_WAKE;
        _os9(F_SEND, &r);
    }
    while (g_rq->tail != g_rq->head) {
        r.rg_x = 0;                  /* sleep until any signal */
        _os9(F_SLEEP, &r);
    }
    return 0;
}

int ren_shut()
{
    struct registers r;

    if (g_rq == 0) return 0;

    /* drain pending work first so child sees quit only after present */
    ren_flush();
    g_rq->quit = 1;

    /* Wake child so it observes quit promptly (it's likely sleeping
     * with no work pending). */
    r.rg_a = (char)g_rpid;
    r.rg_b = (char)SIG_RENDER_WAKE;
    _os9(F_SEND, &r);

    /* F$Wait can return A=0 when interrupted by a signal (e.g., the
     * child's final DONE arriving mid-wait). Loop past that until we
     * actually reap a child. */
    for (;;) {
        if (_os9(F_WAIT, &r)) {
            printf("render: F$Wait err #%d\n", r.rg_b & 0xff);
            break;
        }
        if ((r.rg_a & 0xff) != 0) break;     /* reaped child PID */
    }

    r.rg_b = 1;
    r.rg_u = (unsigned)g_rq;
    _os9(F_CLRBLK, &r);
    r.rg_b = 1;
    r.rg_x = g_rblock;
    _os9(F_DELRAM, &r);

    g_rq = 0;
    g_rblock = -1;
    g_rpid = -1;
    return 0;
}
