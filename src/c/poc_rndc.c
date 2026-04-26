/***********************************************************************
 * poc_rndc.c  -  Render child process (phase 4).
 *
 * Maps the shared RenderQueue from the block# in argv[1], opens a
 * CoVDG VDG path, allocates two 16K (160x192x16) screens, sets
 * ready=1, then polls the queue and executes draw commands on the
 * back screen. R_OP_PRESENT calls SS.DScrn on the back and swaps.
 *
 * Sprites use byte-copy save_bg / rest_bg for their 32-byte footprint.
 * The earlier ghosting bug (save_bg capturing another sprite's pixels
 * under overlap) is prevented at a higher level: apply_pending_and_
 * present runs three passes (clears -> saves -> draws), so every
 * save_bg sees clean post-clear screen. paint_bg_at + tile_color (an
 * earlier per-pixel re-paint approach) was correct but ~5x more
 * expensive per pixel, capping framerate near 11 fps; reverted to
 * byte-copies after benchmarking (lessons-learned 2026-04-26).
 *
 * R_OP_DRAWMAP draws the map to the back buffer, then memcpys to the
 * other screen so both buffers carry the same map after one call —
 * one 16K block copy is ~100x faster than running the procedural
 * draw twice.
 *
 * RenderQueue layout MUST match render.c.
 *
 * Compile: dcc poc_rndc.c -m=8k -f=/dd/cmds/pocrndc
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef I_SETSTT
#define I_SETSTT  0x8E
#endif
#ifndef F_SEND
#define F_SEND    0x08
#endif
#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif
#ifndef F_MAPBLK
#define F_MAPBLK  0x3D
#endif
#ifndef F_CLRBLK
#define F_CLRBLK  0x38
#endif
#ifndef SS_ASCRN
#define SS_ASCRN  0x8B
#endif
#ifndef SS_DSCRN
#define SS_DSCRN  0x8C
#endif
#ifndef SS_FSCRN
#define SS_FSCRN  0x8D
#endif

#define SCR_W      160
#define SCR_H      192
#define SCR_BPR    80
#define SCR_TYPE   2

#define MAP_COLS   9
#define MAP_ROWS   5
#define MAP_OX     2
#define MAP_OY     1
#define TILE_W     17
#define TILE_H     38

#define SPR_W      8
#define SPR_H      8
#define SPR_BW     4
#define SPR_SIZE   (SPR_W * SPR_H)
#define WALK_FR    2

#define SPR_SLOTS  2

#define COLOR_BLACK 0
#define COLOR_GREEN 1
#define COLOR_BLUE  2
#define COLOR_WHITE 3
#define COLOR_PLYR  12
#define COLOR_MULE  14

#define RENDER_MAGIC      0x5244
#define RENDER_QUEUE_SIZE 64
#define RENDER_QUEUE_MASK (RENDER_QUEUE_SIZE - 1)

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
    unsigned char a, b, c;
    unsigned int  x, y;
} RenderCmd;

typedef struct {
    unsigned int magic;
    unsigned int head;
    unsigned int tail;
    unsigned int quit;
    unsigned int ready;
    unsigned int parent_pid;
    RenderCmd    entries[RENDER_QUEUE_SIZE];
} RenderQueue;

extern int intercept();
static int g_lastsig;

static int rd_sigtrap(sig)
int sig;
{
    g_lastsig = sig;
    return 0;
}

extern int open(), close(), write();

static int             g_path;
static char           *g_devname;
static unsigned char  *g_scr[2];

/* Per-(screen, slot) sprite state. We do NOT save the screen bytes
 * under the sprite — that approach captured overlapping-sprite pixels
 * and resurrected them as ghosts on restore. Instead we re-derive the
 * bg from the map via tile_color() / paint_bg_at(). */
static unsigned char   g_bg[2][SPR_SLOTS][SPR_BW * SPR_H];
static int             g_prevx[2][SPR_SLOTS];
static int             g_prevy[2][SPR_SLOTS];
static int             g_have[2][SPR_SLOTS];

/* Pending sprite state for the current frame. R_OP_SPRITE just records
 * here; R_OP_PRESENT applies them in two passes (all clears, then all
 * draws) so a slot's bg-clear can never erase another slot's just-
 * drawn pixels under overlap.
 *
 * Field names use a `pp_` prefix because DCC K&R treats struct member
 * names as a single global namespace — `x` and `y` are already
 * declared `unsigned int` in RenderCmd, and re-declaring them as `int`
 * here was a "struct member mismatch" compile error. */
typedef struct {
    int pp_x, pp_y, pp_frame, pp_dir, pp_mule;
    int pp_active;
} PendingSpr;
static PendingSpr      g_pending[SPR_SLOTS];

/* Embedded map — same as poc_cvdg16. R_OP_DRAWMAP draws this to the
 * current back buffer. Future phase will replace this with shared
 * tilemap state for in-game map mutations. */
static unsigned char g_map[MAP_ROWS][MAP_COLS] = {
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 0, 1, 0, 0, 0, 2 },
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 2, 1, 2, 0, 0, 2 },
    { 0, 2, 0, 0, 1, 0, 0, 2, 0 }
};
static int             g_num[2];
static int             g_back;

/* Sprite art ported from poc_cvdg16.c. Two walk frames each. */
static unsigned char g_puddat[WALK_FR][SPR_SIZE] = {
{
 0,0,0,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0,0,0
},
{
 0,0,0,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0,0,0,0xff,0,0
}
};

static unsigned char g_plrdat[WALK_FR][SPR_SIZE] = {
{
 0,0,0,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0,0xff,0
},
{
 0,0,0,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0xff,0xff,0,0,0,
 0,0xff,0xff,0,0xff,0xff,0,0,
 0,0xff,0,0,0,0xff,0,0
}
};

static unsigned char g_muddat[WALK_FR][SPR_SIZE] = {
{
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0,0,0,
 0,0,0,0,0,0,0,0
},
{
 0,0,0,0xff,0xff,0,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0xff,0xff,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0xff,0,0,0xff,0,0,
 0,0,0,0,0,0xff,0,0,
 0,0,0,0,0,0,0,0
}
};

static unsigned char g_mlrdat[WALK_FR][SPR_SIZE] = {
{
 0,0xff,0xff,0,0xff,0xff,0xff,0,
 0xff,0,0xff,0xff,0xff,0xff,0xff,0xff,
 0,0xff,0xff,0,0xff,0xff,0xff,0xff,
 0,0,0,0,0xff,0,0xff,0,
 0,0,0,0xff,0,0,0xff,0xff,
 0,0,0,0xff,0,0,0,0xff,
 0,0,0xff,0xff,0,0,0xff,0xff,
 0,0,0,0,0,0,0,0
},
{
 0,0xff,0xff,0,0xff,0xff,0xff,0,
 0xff,0,0xff,0xff,0xff,0xff,0xff,0xff,
 0,0xff,0xff,0,0xff,0xff,0xff,0xff,
 0,0,0,0,0xff,0,0xff,0,
 0,0,0,0,0xff,0,0,0xff,
 0,0,0,0,0xff,0,0,0xff,
 0,0,0,0xff,0xff,0,0xff,0xff,
 0,0,0,0,0,0,0,0
}
};

wpath(buf, len)
unsigned char *buf; int len;
{
    write(g_path, buf, len);
}

nap(ticks)
int ticks;
{
    struct registers r;
    r.rg_x = (unsigned)ticks;
    _os9(F_SLEEP, &r);
}

int alloc_screen(pp, np)
unsigned char **pp; int *np;
{
    struct registers r;

    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_ASCRN;
    r.rg_x = (unsigned)SCR_TYPE;
    if (_os9(I_SETSTT, &r)) return -1;
    *pp = (unsigned char *)r.rg_x;
    *np = r.rg_y;
    return 0;
}

show_screen(num)
int num;
{
    struct registers r;
    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_DSCRN;
    r.rg_y = (unsigned)num;
    _os9(I_SETSTT, &r);
}

free_screen(num)
int num;
{
    struct registers r;
    if (num <= 0) return;
    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_FSCRN;
    r.rg_y = (unsigned)num;
    _os9(I_SETSTT, &r);
}

vsel()
{
    unsigned char cmd[2];
    cmd[0] = 0x1b;
    cmd[1] = 0x21;
    wpath(cmd, 2);
    nap(2);
}

int open_vdg()
{
    char *names[6];
    int i;

    names[0] = "/term_vdg";
    names[1] = "/TERM_VDG";
    names[2] = "/verm";
    names[3] = "/VERM";
    names[4] = "/term";
    names[5] = 0;

    for (i = 0; names[i]; i++) {
        g_devname = names[i];
        g_path = open(g_devname, 3);
        if (g_path < 0) continue;
        if (alloc_screen(&g_scr[0], &g_num[0]) == 0) return 0;
        close(g_path);
    }
    g_path = -1;
    return -1;
}

putpx(base, x, y, c)
unsigned char *base; int x, y, c;
{
    unsigned char *p;
    if ((unsigned)x >= SCR_W || (unsigned)y >= SCR_H) return;
    p = base + y * SCR_BPR + (x >> 1);
    if (x & 1)
        *p = (*p & 0xf0) | (c & 15);
    else
        *p = (*p & 0x0f) | ((c & 15) << 4);
}

hline(base, x, y, w, c)
unsigned char *base; int x, y, w, c;
{
    int nb;
    unsigned char *p, *q;
    unsigned char fill, nib;

    if (w <= 0 || y < 0 || y >= SCR_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (w <= 0) return;

    nib  = (unsigned char)(c & 15);
    fill = (nib << 4) | nib;
    p    = base + y * SCR_BPR + (x >> 1);

    /* Leading half-byte if x is odd. */
    if (x & 1) {
        *p = (*p & 0xf0) | nib;
        p++;
        w--;
    }
    /* Whole bytes — tight inner loop, no per-pixel mask. */
    nb = w >> 1;
    q  = p + nb;
    while (p < q) *p++ = fill;
    /* Trailing half-byte if w is odd. */
    if (w & 1) {
        *p = (*p & 0x0f) | (nib << 4);
    }
}

rect(base, x, y, w, h, c)
unsigned char *base; int x, y, w, h, c;
{
    int r;
    for (r = 0; r < h; r++) hline(base, x, y + r, w, c);
}

clear_back(color)
int color;
{
    rect(g_scr[g_back], 0, 0, SCR_W, SCR_H, color);
}

plain(base, x, y)
unsigned char *base; int x, y;
{
    rect(base, x, y, TILE_W, TILE_H, COLOR_GREEN);
    rect(base, x + 2, y + 17, TILE_W - 4, 2, COLOR_BLACK);
    rect(base, x + 5, y + 8, 2, 2, COLOR_BLACK);
    rect(base, x + 11, y + 27, 2, 2, COLOR_BLACK);
}

river(base, x, y)
unsigned char *base; int x, y;
{
    rect(base, x, y, TILE_W, TILE_H, COLOR_BLUE);
    rect(base, x + 6, y, 5, TILE_H, COLOR_WHITE);
    rect(base, x + 7, y, 3, TILE_H, COLOR_BLUE);
    rect(base, x + 5, y + 11, 7, 3, COLOR_BLUE);
    rect(base, x + 5, y + 27, 7, 3, COLOR_BLUE);
}

mountn(base, x, y)
unsigned char *base; int x, y;
{
    rect(base, x, y, TILE_W, TILE_H, COLOR_GREEN);
    rect(base, x + 7, y + 4, 3, 7, COLOR_WHITE);
    rect(base, x + 5, y + 11, 7, 7, COLOR_WHITE);
    rect(base, x + 3, y + 18, 11, 9, COLOR_WHITE);
    rect(base, x + 1, y + 27, 15, 8, COLOR_WHITE);
    rect(base, x + 8, y + 5, 2, 9, COLOR_BLACK);
}

draw_tile(kind, col, row)
int kind, col, row;
{
    unsigned char *base;
    int x, y;

    base = g_scr[g_back];
    x = MAP_OX + col * TILE_W;
    y = MAP_OY + row * TILE_H;
    if (kind == 1)      river(base, x, y);
    else if (kind == 2) mountn(base, x, y);
    else                plain(base, x, y);
}

draw_map_back()
{
    int row, col;
    unsigned char *src, *dst, *end;

    rect(g_scr[g_back], 0, 0, SCR_W, SCR_H, COLOR_BLACK);
    for (row = 0; row < MAP_ROWS; row++)
        for (col = 0; col < MAP_COLS; col++) {
            unsigned char *base;
            int x, y;
            base = g_scr[g_back];
            x = MAP_OX + col * TILE_W;
            y = MAP_OY + row * TILE_H;
            if (g_map[row][col] == 1)      river(base, x, y);
            else if (g_map[row][col] == 2) mountn(base, x, y);
            else                           plain(base, x, y);
        }

    /* Clone to the OTHER screen so both buffers carry the same map.
     * One 16K memcpy is ~100x faster than re-running the full
     * procedural draw pass on the second screen. */
    src = g_scr[g_back];
    dst = g_scr[g_back ^ 1];
    end = src + SCR_BPR * SCR_H;
    while (src < end) *dst++ = *src++;

    /* Map redrawn on both screens — any saved sprite-prev state is
     * stale (the prev location now shows clean map). */
    g_have[0][0] = 0; g_have[0][1] = 0;
    g_have[1][0] = 0; g_have[1][1] = 0;
}

/* save_bg / rest_bg: byte-copy the SPR_BW x SPR_H rectangle of screen
 * bytes covering the sprite footprint into / out of a slot's bg
 * buffer. Sprites are always positioned at even x, so (x >> 1) is
 * byte-aligned and a row is exactly SPR_BW = 4 bytes.
 *
 * The three-pass apply_pending_and_present pipeline (clears -> saves
 * -> draws) guarantees save_bg only runs on clean post-restore screen
 * state, so what gets captured is never another sprite's pixels. */
save_bg(base, x, y, buf)
unsigned char *base; int x, y; unsigned char *buf;
{
    int row, col, idx;
    unsigned char *p;

    idx = 0;
    for (row = 0; row < SPR_H; row++) {
        p = base + (y + row) * SCR_BPR + (x >> 1);
        for (col = 0; col < SPR_BW; col++) buf[idx++] = *p++;
    }
}

rest_bg(base, x, y, buf)
unsigned char *base; int x, y; unsigned char *buf;
{
    int row, col, idx;
    unsigned char *p;

    idx = 0;
    for (row = 0; row < SPR_H; row++) {
        p = base + (y + row) * SCR_BPR + (x >> 1);
        for (col = 0; col < SPR_BW; col++) *p++ = buf[idx++];
    }
}

/* Repaint the SPR_W x SPR_H footprint at (x, y) from map data only.
 * Used to clear a sprite before its next position is drawn — never
 * samples the live screen, so overlapping sprites cannot contaminate
 * each other's bg.
 *
 * Optimization: K&R DCC `int` division is hundreds of cycles. The
 * naive form did 2 div + 2 mod per pixel = 256 expensive ops per
 * call, dominating the per-frame budget. Since the sprite is only
 * SPR_W=8 wide and SPR_H=8 tall, `col` and `row` change at most once
 * across the whole footprint. Compute starting (col,row,tx,ty) once
 * per sprite and bump indices in the inner loop with cheap compare-
 * and-reset. ~5-10x speedup on this function in practice. */
paint_bg_at(base, x, y)
unsigned char *base; int x, y;
{
    int sx, sy;
    int row, col0, ty, tx0;
    int row_x, col_x, ty_x, tx_x;        /* working copies */
    int kind, c;
    int my, mx;

    /* row / ty for the top scanline of the sprite. */
    my = y - MAP_OY;
    if (my < 0 || my >= MAP_ROWS * TILE_H) {
        row = -1;
        ty  = 0;
    } else {
        row = 0;
        while ((row + 1) * TILE_H <= my) row++;
        ty = my - row * TILE_H;
    }

    /* col / tx for the leftmost pixel of the sprite. */
    mx = x - MAP_OX;
    if (mx < 0 || mx >= MAP_COLS * TILE_W) {
        col0 = -1;
        tx0  = 0;
    } else {
        col0 = 0;
        while ((col0 + 1) * TILE_W <= mx) col0++;
        tx0 = mx - col0 * TILE_W;
    }

    row_x = row;
    ty_x  = ty;
    for (sy = 0; sy < SPR_H; sy++) {
        col_x = col0;
        tx_x  = tx0;
        for (sx = 0; sx < SPR_W; sx++) {
            if (row_x < 0 || row_x >= MAP_ROWS ||
                col_x < 0 || col_x >= MAP_COLS) {
                c = COLOR_BLACK;
            } else {
                kind = (int)g_map[row_x][col_x];
                c = tile_color(kind, tx_x, ty_x);
            }
            putpx(base, x + sx, y + sy, c);
            tx_x++;
            if (tx_x == TILE_W) { tx_x = 0; col_x++; }
        }
        ty_x++;
        if (ty_x == TILE_H) { ty_x = 0; row_x++; }
    }
}

/* drawspr: write the 8x8 sprite to (x,y) processing two pixels per
 * byte. Sprites are positioned at even x (enforced by evenx() in the
 * caller's pathpos), so (x>>1) is byte-aligned and a row is exactly
 * SPR_BW = 4 output bytes. Per output byte we look at two source
 * bytes from `dat` (one per pixel; nonzero = opaque):
 *
 *   both opaque  -> *p = colorbyte             (one store, no read)
 *   high only    -> *p = (*p & 0x0F) | colorhi (read-modify-write)
 *   low only     -> *p = (*p & 0xF0) | colorlo (read-modify-write)
 *   both empty   -> skip                       (no syscall, no write)
 *
 * The `flip` flag mirrors the sprite horizontally — handled by walking
 * the source pointer backwards. ~3x faster than the per-pixel putpx
 * form because the common case (both opaque or both transparent)
 * collapses to a single byte op instead of two read-modify-write
 * cycles. */
drawspr(base, x, y, dat, color, flip)
unsigned char *base; int x, y; unsigned char *dat; int color, flip;
{
    unsigned char *p, *srow;
    int r, c;
    int ci;
    unsigned char colorbyte, colorhi, colorlo;
    int hi, lo;

    ci = color & 15;
    colorbyte = (unsigned char)((ci << 4) | ci);
    colorhi   = (unsigned char)(ci << 4);
    colorlo   = (unsigned char)ci;

    for (r = 0; r < SPR_H; r++) {
        p = base + (y + r) * SCR_BPR + (x >> 1);
        if (flip) {
            srow = dat + r * SPR_W + (SPR_W - 1);
            for (c = 0; c < SPR_BW; c++) {
                hi = srow[0];
                lo = srow[-1];
                srow -= 2;
                if (hi) {
                    if (lo) *p = colorbyte;
                    else    *p = (*p & 0x0f) | colorhi;
                } else if (lo) {
                    *p = (*p & 0xf0) | colorlo;
                }
                p++;
            }
        } else {
            srow = dat + r * SPR_W;
            for (c = 0; c < SPR_BW; c++) {
                hi = srow[0];
                lo = srow[1];
                srow += 2;
                if (hi) {
                    if (lo) *p = colorbyte;
                    else    *p = (*p & 0x0f) | colorhi;
                } else if (lo) {
                    *p = (*p & 0xf0) | colorlo;
                }
                p++;
            }
        }
    }
}

/* Record a sprite update for the current frame. Actual drawing is
 * deferred to apply_pending_and_present() so we can do all clears
 * before any draws — otherwise slot 1's bg-clear at its prev position
 * could land on slot 0's just-drawn pixels and erase them. */
record_sprite(slot, x, y, frame, dir, mule)
int slot, x, y, frame, dir, mule;
{
    if ((unsigned)slot >= SPR_SLOTS) return;
    g_pending[slot].pp_x      = x;
    g_pending[slot].pp_y      = y;
    g_pending[slot].pp_frame  = frame;
    g_pending[slot].pp_dir    = dir;
    g_pending[slot].pp_mule   = mule;
    g_pending[slot].pp_active = 1;
}

draw_one_sprite(base, slot)
unsigned char *base; int slot;
{
    PendingSpr *s;
    int walk, flip;
    unsigned char *dat;

    s = &g_pending[slot];
    walk = s->pp_frame & 1;
    flip = 0;
    if (!s->pp_mule) {
        if (s->pp_dir == 0 || s->pp_dir == 2) {
            dat = &g_plrdat[walk][0];
            if (s->pp_dir == 0) flip = 1;
        } else {
            dat = &g_puddat[walk][0];
        }
        drawspr(base, s->pp_x, s->pp_y, dat, COLOR_PLYR, flip);
    } else {
        if (s->pp_dir == 0 || s->pp_dir == 2) {
            dat = &g_mlrdat[walk][0];
            if (s->pp_dir == 0) flip = 1;
        } else {
            dat = &g_muddat[walk][0];
        }
        drawspr(base, s->pp_x, s->pp_y, dat, COLOR_MULE, flip);
    }
}

apply_pending_and_present()
{
    unsigned char *base;
    int s;

    base = g_scr[g_back];

    /* Pass 1 (clears): rest_bg every slot whose prev position on this
     * screen has a saved bg. Cheap byte-copy. After this pass the
     * screen is clean (just the map) at every prev sprite footprint. */
    for (s = 0; s < SPR_SLOTS; s++) {
        if (g_have[g_back][s]) {
            rest_bg(base, g_prevx[g_back][s], g_prevy[g_back][s],
                    &g_bg[g_back][s][0]);
        }
    }

    /* Pass 2 (saves): for each pending sprite, save the bg at its NEW
     * position before any drawspr runs. Pass 1 cleared the screen at
     * old footprints; no draw has touched the new footprint yet — so
     * save_bg captures clean map bytes, never another sprite's
     * pixels. This is the invariant that lets byte-copy save_bg work
     * correctly under arbitrary sprite overlap. */
    for (s = 0; s < SPR_SLOTS; s++) {
        if (g_pending[s].pp_active) {
            save_bg(base, g_pending[s].pp_x, g_pending[s].pp_y,
                    &g_bg[g_back][s][0]);
        }
    }

    /* Pass 3 (draws + bookkeeping). */
    for (s = 0; s < SPR_SLOTS; s++) {
        if (g_pending[s].pp_active) {
            draw_one_sprite(base, s);
            g_prevx[g_back][s] = g_pending[s].pp_x;
            g_prevy[g_back][s] = g_pending[s].pp_y;
            g_have[g_back][s]  = 1;
            g_pending[s].pp_active = 0;
        }
    }

    /* Pass 4: flip. */
    show_screen(g_num[g_back]);
    g_back ^= 1;
}

main(argc, argv)
int argc;
char *argv[];
{
    struct registers r;
    RenderQueue *q;
    RenderCmd   *e;
    int block;

    if (argc < 2) { printf("pocrndc: need block#\n"); exit(1); }
    block = atoi(argv[1]);
    printf("pocrndc: started block=%d\n", block);

    r.rg_b = 1;
    r.rg_x = block;
    if (_os9(F_MAPBLK, &r)) {
        printf("pocrndc: F$MapBlk b=%d err #%d\n", block, r.rg_b & 0xff);
        exit(2);
    }
    q = (RenderQueue *)r.rg_u;

    if ((int)q->magic != RENDER_MAGIC) {
        printf("pocrndc: bad magic %x\n", (int)q->magic);
        exit(3);
    }

    g_path = -1;
    g_num[0] = 0;
    g_num[1] = 0;
    /* Start with scr[0] showing and scr[1] as the back buffer, so the
     * very first R_OP_DRAWMAP draws into the OFFSCREEN buffer. With
     * g_back=0 the first map redraw was happening on the visible
     * screen and the user could see tiles paint in line-by-line. */
    g_back = 1;

    if (open_vdg()) {
        printf("pocrndc: no CoVDG path\n");
        exit(4);
    }
    if (alloc_screen(&g_scr[1], &g_num[1])) {
        printf("pocrndc: second screen alloc failed\n");
        free_screen(g_num[0]);
        close(g_path);
        exit(5);
    }

    /* Signal ready as soon as both screens are bound. The parent only
     * needs to know the queue is mapped + the child can accept work;
     * the cosmetic prep below (vsel/clear/show) used to run before
     * `ready` and could exceed the 5s init budget on slower emulators
     * because two software rects are ~30K putpx calls. Move it after
     * `ready` instead — the first drained command will be ren_clr()
     * which redraws the back buffer anyway. */
    /* Install signal trap so SIG_RENDER_WAKE doesn't kill us; it just
     * sets g_lastsig and aborts whatever F$Sleep is in flight. */
    g_lastsig = 0;
    intercept(rd_sigtrap);

    q->ready = 1;

    vsel();
    rect(g_scr[0], 0, 0, SCR_W, SCR_H, COLOR_BLACK);
    rect(g_scr[1], 0, 0, SCR_W, SCR_H, COLOR_BLACK);
    show_screen(g_num[0]);
    nap(2);

    for (;;) {
        while (q->tail != q->head) {
            e = &q->entries[q->tail];
            switch (e->op) {
            case R_OP_CLEAR:
                clear_back((int)e->a);
                break;
            case R_OP_TILE:
                draw_tile((int)e->a, (int)e->b, (int)e->c);
                break;
            case R_OP_SPRITE:
                /* a=slot  b=frame  c=(dir | mule<<4)  x=px  y=py
                 * Just records pending — actual draw happens in PRESENT. */
                record_sprite((int)e->a, (int)e->x, (int)e->y,
                              (int)e->b,
                              (int)(e->c & 0x0f),
                              (int)((e->c >> 4) & 1));
                break;
            case R_OP_PRESENT:
                apply_pending_and_present();
                break;
            case R_OP_DRAWMAP:
                draw_map_back();
                break;
            case R_OP_PALETTE:
                /* deferred to a later phase; SS.PalSet codepath
                 * unconfirmed against EOU windowing manual */
                break;
            }
            q->tail = (q->tail + 1) & RENDER_QUEUE_MASK;
        }
        if (q->quit) break;

        /* Notify parent that we've drained, so its ren_flush can wake.
         * Then sleep indefinitely until parent F$Sends WAKE. */
        if (q->parent_pid) {
            r.rg_a = (char)q->parent_pid;
            r.rg_b = (char)SIG_RENDER_DONE;
            _os9(F_SEND, &r);
        }
        r.rg_x = 0;                  /* sleep until any signal */
        _os9(F_SLEEP, &r);
    }

    show_screen(0);
    free_screen(g_num[0]);
    free_screen(g_num[1]);
    close(g_path);

    r.rg_b = 1;
    r.rg_u = (unsigned)q;
    _os9(F_CLRBLK, &r);

    exit(0);
}
