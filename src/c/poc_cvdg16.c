/***********************************************************************
 * poc_cvdg16.c - NitrOS-9 CoVDG 160x192x16 page-flip PoC
 *
 * Uses documented CoVDG SS.AScrn / SS.DScrn / SS.FScrn calls.
 * Screen type 2 is 160x192x16 colors and occupies 16K.
 *
 * Compile: dcc poc_cvdg16.c -s -m=4k -f=/dd/cmds/poccvd16
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define SCR_W      160
#define SCR_H      192
#define SCR_BPR    80
#define SCR_TYPE   2
#define NFRAMES    150
#define MAP_COLS   9
#define MAP_ROWS   5
#define TILE_W     17
#define TILE_H     38
#define MAP_OX     2
#define MAP_OY     1
#define SPR_W      9
#define SPR_H      12
#define SPR_BW     5
#define SPR_SIZE   (SPR_BW * SPR_H)
#define WALK_FR    4
#define PLYR_Y     (MAP_OY + 8)
#define MULE_Y     (MAP_OY + TILE_H * 3)

#define COLOR_BLACK 0
#define COLOR_GREEN 1
#define COLOR_BLUE  2
#define COLOR_WHITE 3
#define COLOR_PLYR  12
#define COLOR_MULE  14

#ifndef I_SETSTT
#define I_SETSTT   0x8E
#endif
#ifndef F_SLEEP
#define F_SLEEP    0x0A
#endif
#ifndef F_TIME
#define F_TIME     0x15
#endif
#ifndef SS_ASCRN
#define SS_ASCRN   0x8B
#endif
#ifndef SS_DSCRN
#define SS_DSCRN   0x8C
#endif
#ifndef SS_FSCRN
#define SS_FSCRN   0x8D
#endif

int open(), write(), close();

static int g_path;
static unsigned char *g_scr1;
static unsigned char *g_scr2;
static int g_num1;
static int g_num2;
static char *g_devname;
static unsigned char g_bg1p[SPR_BW * SPR_H];
static unsigned char g_bg1m[SPR_BW * SPR_H];
static unsigned char g_bg2p[SPR_BW * SPR_H];
static unsigned char g_bg2m[SPR_BW * SPR_H];

static unsigned char g_map[MAP_ROWS][MAP_COLS] = {
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 0, 1, 0, 0, 0, 2 },
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 2, 1, 2, 0, 0, 2 },
    { 0, 2, 0, 0, 1, 0, 0, 2, 0 }
};

static unsigned char g_pmsk[SPR_SIZE] = {
    0x00,0xf0,0xf0,0x00,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0x00,0xf0,0xf0,0x00,0x00,
    0x00,0xf0,0xf0,0x00,0x00,
    0x00,0xf0,0xf0,0x00,0x00,
    0xff,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0xff,0x00,
    0x0f,0xf0,0x00,0xff,0x00,
    0x0f,0xf0,0x00,0xff,0x00
};

static unsigned char g_pdat[WALK_FR][SPR_SIZE] = {
{
    0x00,0xcc,0xc0,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0x33,0x3c,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0x0c,0xc0,0x00,0xcc,0x00,
    0x0c,0xc0,0x00,0xcc,0x00
},
{
    0x00,0xcc,0xc0,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0x33,0x3c,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0x0c,0xc0,0x00,0x0c,0xc0,
    0x0c,0xc0,0x00,0x0c,0xc0
},
{
    0x00,0xcc,0xc0,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0x33,0x3c,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0x0c,0xc0,0x00,0xcc,0x00,
    0x0c,0xc0,0x00,0xcc,0x00
},
{
    0x00,0xcc,0xc0,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0xcc,0xcc,0x00,0x00,
    0x0c,0x33,0x3c,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0x00,0xcc,0xc0,0x00,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0xcc,0xcc,0xcc,0xcc,0x00,
    0x00,0xcc,0x00,0xcc,0x00,
    0x00,0xcc,0x00,0xcc,0x00
}
};

static unsigned char g_mmsk[SPR_SIZE] = {
    0x00,0x00,0x00,0xff,0x00,
    0x00,0x00,0x00,0xff,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0x0f,0xff,0xff,0x00,0x00,
    0xff,0xff,0xff,0xff,0xf0,
    0xff,0xff,0xff,0xff,0xf0,
    0xff,0xff,0xff,0xff,0xf0,
    0xff,0x00,0x00,0x0f,0xf0,
    0xff,0x00,0x00,0x0f,0xf0,
    0xff,0x00,0x00,0x0f,0xf0
};

static unsigned char g_mdat[WALK_FR][SPR_SIZE] = {
{
    0x00,0x00,0x00,0xee,0x00,
    0x00,0x00,0x00,0xe3,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0x00,0x00,0x0e,0xe0,
    0xee,0x00,0x00,0x0e,0xe0,
    0xee,0x00,0x00,0x0e,0xe0
},
{
    0x00,0x00,0x00,0xee,0x00,
    0x00,0x00,0x00,0xe3,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0x0e,0xe0,0x00,0xee,0x00,
    0x0e,0xe0,0x00,0xee,0x00,
    0x0e,0xe0,0x00,0xee,0x00
},
{
    0x00,0x00,0x00,0xee,0x00,
    0x00,0x00,0x00,0xe3,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0x00,0x00,0x0e,0xe0,
    0xee,0x00,0x00,0x0e,0xe0,
    0xee,0x00,0x00,0x0e,0xe0
},
{
    0x00,0x00,0x00,0xee,0x00,
    0x00,0x00,0x00,0xe3,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0x0e,0xee,0xee,0x00,0x00,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xee,0xee,0xee,0xee,0xe0,
    0xe0,0x00,0x00,0x0e,0xe0,
    0xe0,0x00,0x00,0x0e,0xe0,
    0xe0,0x00,0x00,0x0e,0xe0
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

int nowsec()
{
    struct registers r;
    unsigned char pkt[6];

    r.rg_x = (unsigned)pkt;
    if (_os9(F_TIME, &r)) return -1;
    return ((int)pkt[4]) * 60 + (int)pkt[5];
}

int alloc_screen(pp, np)
unsigned char **pp; int *np;
{
    struct registers r;

    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_ASCRN;
    r.rg_x = (unsigned)SCR_TYPE;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_cvdg16: %s SS.AScrn error #%d\n",
               g_devname, r.rg_b & 0xff);
        return -1;
    }

    *pp = (unsigned char *)r.rg_x;
    *np = r.rg_y;
    printf("poc_cvdg16: %s screen %d at %x\n",
           g_devname, *np, (unsigned)*pp);
    return 0;
}

int show_screen(num)
int num;
{
    struct registers r;

    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_DSCRN;
    r.rg_y = (unsigned)num;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_cvdg16: SS.DScrn %d error #%d\n", num, r.rg_b & 0xff);
        return -1;
    }
    return 0;
}

vsel()
{
    unsigned char cmd[2];

    cmd[0] = 0x1b;
    cmd[1] = 0x21;
    wpath(cmd, 2);
    nap(2);
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
        if (g_path < 0) {
            printf("poc_cvdg16: %s open failed\n", g_devname);
        } else {
            if (alloc_screen(&g_scr1, &g_num1) == 0) return 0;
            close(g_path);
        }
    }

    g_path = -1;
    g_devname = "";
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

hline(base, x, y, w, c)
unsigned char *base; int x, y, w, c;
{
    int i;

    if (w <= 0 || y < 0 || y >= SCR_H) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > SCR_W) w = SCR_W - x;
    for (i = 0; i < w; i++) putpx(base, x + i, y, c);
}

rect(base, x, y, w, h, c)
unsigned char *base; int x, y, w, h, c;
{
    int r;

    for (r = 0; r < h; r++) hline(base, x, y + r, w, c);
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

tile(base, kind, x, y)
unsigned char *base; int kind, x, y;
{
    if (kind == 1)
        river(base, x, y);
    else if (kind == 2)
        mountn(base, x, y);
    else
        plain(base, x, y);
}

mapdraw(base)
unsigned char *base;
{
    int r, c;

    rect(base, 0, 0, SCR_W, SCR_H, COLOR_BLACK);
    for (r = 0; r < MAP_ROWS; r++)
        for (c = 0; c < MAP_COLS; c++)
            tile(base, g_map[r][c],
                 MAP_OX + c * TILE_W,
                 MAP_OY + r * TILE_H);
}

drawspr(base, x, y, msk, dat)
unsigned char *base; int x, y; unsigned char *msk, *dat;
{
    int r, c, i;
    unsigned char *p;
    unsigned char m;

    i = 0;
    for (r = 0; r < SPR_H; r++) {
        p = base + (y + r) * SCR_BPR + (x >> 1);
        for (c = 0; c < SPR_BW; c++) {
            m = msk[i];
            *p = (*p & ~m) | (dat[i] & m);
            p++;
            i++;
        }
    }
}

render_step(base, frame, old1p, old2p, bgp, bgm)
unsigned char *base; int frame, *old1p, *old2p;
unsigned char *bgp, *bgm;
{
    int sx1, sx2;

    sx1 = MAP_OX + (frame * 2) % (MAP_COLS * TILE_W - SPR_W);
    sx2 = MAP_OX + MAP_COLS * TILE_W - SPR_W
          - (frame * 2) % (MAP_COLS * TILE_W - SPR_W);

    rest_bg(base, *old1p, PLYR_Y, bgp);
    rest_bg(base, *old2p, MULE_Y, bgm);
    save_bg(base, sx1, PLYR_Y, bgp);
    save_bg(base, sx2, MULE_Y, bgm);
    drawspr(base, sx1, PLYR_Y, g_pmsk, g_pdat[frame & 3]);
    drawspr(base, sx2, MULE_Y, g_mmsk, g_mdat[(frame + 2) & 3]);

    *old1p = sx1;
    *old2p = sx2;
}

animate()
{
    int frame, start, end, elapsed;
    int old11, old12, old21, old22;

    old11 = MAP_OX;
    old12 = MAP_OX + MAP_COLS * TILE_W - SPR_W;
    old21 = old11;
    old22 = old12;
    mapdraw(g_scr1);
    mapdraw(g_scr2);
    save_bg(g_scr1, old11, PLYR_Y, g_bg1p);
    save_bg(g_scr1, old12, MULE_Y, g_bg1m);
    save_bg(g_scr2, old21, PLYR_Y, g_bg2p);
    save_bg(g_scr2, old22, MULE_Y, g_bg2m);
    drawspr(g_scr1, old11, PLYR_Y, g_pmsk, g_pdat[0]);
    drawspr(g_scr1, old12, MULE_Y, g_mmsk, g_mdat[0]);
    drawspr(g_scr2, old21, PLYR_Y, g_pmsk, g_pdat[0]);
    drawspr(g_scr2, old22, MULE_Y, g_mmsk, g_mdat[0]);
    show_screen(g_num1);
    printf("poc_cvdg16: displayed initial frame on %s screen %d\n",
           g_devname, g_num1);
    nap(2);

    start = nowsec();
    printf("poc_cvdg16: 4-frame packed sprite+flip speed test\n");
    for (frame = 0; frame < NFRAMES; frame++) {
        if (frame & 1) {
            render_step(g_scr2, frame, &old21, &old22, g_bg2p, g_bg2m);
            show_screen(g_num2);
        } else {
            render_step(g_scr1, frame, &old11, &old12, g_bg1p, g_bg1m);
            show_screen(g_num1);
        }
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0)
            printf("poc_cvdg16: packed frames=%d seconds=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        else
            printf("poc_cvdg16: packed frames=%d seconds=<1\n", frame);
    }
}

main()
{
    g_path = -1;
    g_num1 = 0;
    g_num2 = 0;

    printf("poc_cvdg16: probing CoVDG VDG descriptors\n");
    if (open_vdg()) {
        fprintf(stderr, "poc_cvdg16: no CoVDG path accepted SS.AScrn\n");
        exit(1);
    }

    printf("poc_cvdg16: allocating second 16K screen\n");
    if (alloc_screen(&g_scr2, &g_num2)) {
        free_screen(g_num1);
        close(g_path);
        exit(1);
    }

    vsel();
    animate();
    show_screen(0);
    free_screen(g_num1);
    free_screen(g_num2);
    close(g_path);
    exit(0);
}
