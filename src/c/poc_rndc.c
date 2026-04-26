/***********************************************************************
 * poc_rndc.c  -  Render child process (phase 4).
 *
 * Maps the shared RenderQueue from the block# in argv[1], opens a
 * CoVDG VDG path, allocates two 16K (160x192x16) screens, sets
 * ready=1, then polls the queue and executes draw commands on the
 * back screen. R_OP_PRESENT calls SS.DScrn on the back and swaps.
 *
 * Sprites use save_bg / rest_bg per-(screen,slot) so that only the
 * sprite footprint is touched per frame — same trick as poc_cvdg16.
 * Full-redraw of 45 tiles + clear was ~100x too slow in software at
 * 160x192x16 (lessons-learned 2026-04-26).
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
    RenderCmd    entries[RENDER_QUEUE_SIZE];
} RenderQueue;

extern int open(), close(), write();

static int             g_path;
static char           *g_devname;
static unsigned char  *g_scr[2];

/* Per-(screen, slot) sprite state for save_bg/rest_bg.
 * 2 screens x 2 slots x 32 bytes = 128 B of bg buffer. */
static unsigned char   g_bg[2][SPR_SLOTS][SPR_BW * SPR_H];
static int             g_prevx[2][SPR_SLOTS];
static int             g_prevy[2][SPR_SLOTS];
static int             g_have[2][SPR_SLOTS];

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
    int i;
    if (w <= 0 || y < 0 || y >= SCR_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    for (i = 0; i < w; i++) putpx(base, x + i, y, c);
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
    /* New map drawn on this screen — invalidate any saved bg under
     * the previous sprite positions on this screen. */
    g_have[g_back][0] = 0;
    g_have[g_back][1] = 0;
}

save_bg(base, x, y, buf)
unsigned char *base; int x, y; unsigned char *buf;
{
    int r, c, i;
    unsigned char *p;

    i = 0;
    for (r = 0; r < SPR_H; r++) {
        p = base + (y + r) * SCR_BPR + (x >> 1);
        for (c = 0; c < SPR_BW; c++) buf[i++] = *p++;
    }
}

rest_bg(base, x, y, buf)
unsigned char *base; int x, y; unsigned char *buf;
{
    int r, c, i;
    unsigned char *p;

    i = 0;
    for (r = 0; r < SPR_H; r++) {
        p = base + (y + r) * SCR_BPR + (x >> 1);
        for (c = 0; c < SPR_BW; c++) *p++ = buf[i++];
    }
}

drawspr(base, x, y, dat, color, flip)
unsigned char *base; int x, y; unsigned char *dat; int color, flip;
{
    int r, c, si;
    for (r = 0; r < SPR_H; r++) {
        for (c = 0; c < SPR_W; c++) {
            if (flip) si = r * SPR_W + (SPR_W - 1 - c);
            else      si = r * SPR_W + c;
            if (dat[si]) putpx(base, x + c, y + r, color);
        }
    }
}

draw_sprite(slot, x, y, frame, dir, mule)
int slot, x, y, frame, dir, mule;
{
    int walk, flip;
    unsigned char *dat;
    unsigned char *base;
    unsigned char *bgbuf;

    if ((unsigned)slot >= SPR_SLOTS) return;

    base  = g_scr[g_back];
    bgbuf = &g_bg[g_back][slot][0];

    /* Restore the background from THIS screen's last sprite position
     * for this slot (if any). Each screen tracks its own trail because
     * page-flip means we see screen N+2's stale state when we come
     * back to screen N. */
    if (g_have[g_back][slot]) {
        rest_bg(base, g_prevx[g_back][slot], g_prevy[g_back][slot], bgbuf);
    }
    save_bg(base, x, y, bgbuf);

    walk = frame & 1;
    flip = 0;
    if (!mule) {
        if (dir == 0 || dir == 2) {
            dat = &g_plrdat[walk][0];
            if (dir == 0) flip = 1;
        } else {
            dat = &g_puddat[walk][0];
        }
        drawspr(base, x, y, dat, COLOR_PLYR, flip);
    } else {
        if (dir == 0 || dir == 2) {
            dat = &g_mlrdat[walk][0];
            if (dir == 0) flip = 1;
        } else {
            dat = &g_muddat[walk][0];
        }
        drawspr(base, x, y, dat, COLOR_MULE, flip);
    }

    g_prevx[g_back][slot] = x;
    g_prevy[g_back][slot] = y;
    g_have[g_back][slot]  = 1;
}

present_back()
{
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
    g_back = 0;

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
                /* a=slot  b=frame  c=(dir | mule<<4)  x=px  y=py */
                draw_sprite((int)e->a, (int)e->x, (int)e->y,
                            (int)e->b,
                            (int)(e->c & 0x0f),
                            (int)((e->c >> 4) & 1));
                break;
            case R_OP_PRESENT:
                present_back();
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
        r.rg_x = 1;
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
