/***********************************************************************
 * poc_tiles.c  -  NitrOS-9 EOU / CoCo 3 partial-copy tile PoC
 *
 * Uses SS.ScInf + F$MapBlk after diagnostics to map a type 6
 * 320x200 4-color screen. Renders each animation frame into a full
 * off-screen buffer, then copies only scanline ranges touched by the
 * moving markers.
 *
 * Type 8 is intentionally not used here: a 320x200 16-color screen is
 * 32K, and a mapped 32K screen plus a 32K back buffer does not leave
 * enough process address space for this C test.
 *
 * Compile: dcc poc_tiles.c -s -m=8k -f=/dd/cmds/poctiles
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

#define SCR_W     320
#define SCR_H     200
#define SCR_BPR   80
#define SCR_SIZE  16000
#define SCR_TYPE  6
#define SCR_COLS  40
#define SCR_ROWS  25

#define TILE_W    35
#define TILE_H    40
#define MAP_COLS  9
#define MAP_ROWS  5
#define MAP_OX    2
#define MAP_OY    0
#define SPR_W     18
#define SPR_H     24

#define COLOR_BLACK 0
#define COLOR_GREEN 1
#define COLOR_BLUE  2
#define COLOR_WHITE 3

#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif
#ifndef F_TIME
#define F_TIME    0x15
#endif
#ifndef I_GETSTT
#define I_GETSTT  0x8D
#endif
#ifndef SS_SCINF
#define SS_SCINF  0x8F
#endif
#ifndef F_MAPBLK
#define F_MAPBLK  0x4F
#endif
#ifndef F_CLRBLK
#define F_CLRBLK  0x50
#endif

int open(), write(), close();

static int g_win;
static unsigned char *g_scr;
static unsigned char *g_back;
static int g_nblks;
static int g_sblk;
static int g_off;

static unsigned char g_map[MAP_ROWS][MAP_COLS] = {
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 0, 1, 0, 0, 0, 2 },
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 2, 1, 2, 0, 0, 2 },
    { 0, 2, 0, 0, 1, 0, 0, 2, 0 }
};

wrwin(buf, len)
unsigned char *buf; int len;
{
    write(g_win, buf, len);
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

selwin()
{
    unsigned char cmd[2];
    cmd[0] = 0x1b;
    cmd[1] = 0x21;
    wrwin(cmd, 2);
}

curhide()
{
    unsigned char cmd[4];
    cmd[0] = 0x1b;
    cmd[1] = 0x39;
    cmd[2] = 0;
    cmd[3] = 0;
    wrwin(cmd, 4);
}

scale0()
{
    unsigned char cmd[3];
    cmd[0] = 0x1b;
    cmd[1] = 0x35;
    cmd[2] = 0;
    wrwin(cmd, 3);
}

palset(prn, ctn)
int prn, ctn;
{
    unsigned char cmd[4];
    cmd[0] = 0x1b;
    cmd[1] = 0x31;
    cmd[2] = (unsigned char)prn;
    cmd[3] = (unsigned char)ctn;
    wrwin(cmd, 4);
}

palinit()
{
    palset(COLOR_BLACK, 0x00);
    palset(COLOR_GREEN, 0x12);
    palset(COLOR_BLUE,  0x09);
    palset(COLOR_WHITE, 0x3f);
}

int open_window()
{
    unsigned char cmd[10];

    g_win = open("/w7", 3);
    if (g_win < 0) return -1;

    cmd[0] = 0x1b;
    cmd[1] = 0x20;
    cmd[2] = SCR_TYPE;
    cmd[3] = 0;
    cmd[4] = 0;
    cmd[5] = SCR_COLS;
    cmd[6] = SCR_ROWS;
    cmd[7] = COLOR_WHITE;
    cmd[8] = COLOR_BLACK;
    cmd[9] = COLOR_BLACK;
    wrwin(cmd, 10);
    nap(2);
    selwin();
    nap(2);
    curhide();
    scale0();
    return 0;
}

int scinfo()
{
    struct registers r;
    int wx, ww, wy, wh;

    r.rg_a = (char)g_win;
    r.rg_b = (char)SS_SCINF;
    if (_os9(I_GETSTT, &r)) {
        printf("poc_tiles: SS.ScInf error #%d\n", r.rg_b & 0xff);
        return -1;
    }

    g_nblks = r.rg_a & 0xff;
    g_sblk = r.rg_b & 0xff;
    g_off = r.rg_x;
    wx = (r.rg_y >> 8) & 0xff;
    ww = r.rg_y & 0xff;
    wy = (r.rg_u >> 8) & 0xff;
    wh = r.rg_u & 0xff;

    printf("poc_tiles: SS.ScInf nb=%d sb=%d off=%x\n",
           g_nblks, g_sblk, g_off);
    printf("poc_tiles: cells x=%d w=%d y=%d h=%d\n",
           wx, ww, wy, wh);

    if (g_nblks != 2 || g_off != 0 || wx != 0 || ww != 40 ||
        wy != 0 || wh != 25) {
        printf("poc_tiles: unsupported screen mapping\n");
        return -1;
    }

    return 0;
}

int mapwin()
{
    struct registers r;

    r.rg_b = (char)g_nblks;
    r.rg_x = (unsigned)g_sblk;
    if (_os9(F_MAPBLK, &r)) {
        printf("poc_tiles: F$MapBlk error #%d\n", r.rg_b & 0xff);
        return -1;
    }

    g_scr = (unsigned char *)(r.rg_u + g_off);
    return 0;
}

unmapwin()
{
    struct registers r;
    r.rg_b = (char)g_nblks;
    r.rg_u = (unsigned)(g_scr - g_off);
    _os9(F_CLRBLK, &r);
}

putpx(x, y, c)
int x, y, c;
{
    unsigned char *p;
    int sh;

    if ((unsigned)x >= SCR_W || (unsigned)y >= SCR_H) return;
    p = g_back + y * SCR_BPR + (x >> 2);
    sh = 6 - ((x & 3) << 1);
    *p = (*p & ~(3 << sh)) | ((c & 3) << sh);
}

hline(x, y, w, c)
int x, y, w, c;
{
    unsigned char pk, *p;
    int i, n;

    if (w <= 0 || y < 0 || y >= SCR_H) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > SCR_W) w = SCR_W - x;
    if (w <= 0) return;

    if (((x | w) & 3) == 0) {
        pk = c & 3;
        pk |= pk << 2;
        pk |= pk << 4;
        p = g_back + y * SCR_BPR + (x >> 2);
        n = w >> 2;
        while (n--) *p++ = pk;
    } else {
        for (i = 0; i < w; i++) putpx(x + i, y, c);
    }
}

rect(x, y, w, h, color)
int x, y, w, h, color;
{
    int r;
    if (w <= 0 || h <= 0) return;
    for (r = 0; r < h; r++) hline(x, y + r, w, color);
}

plain(x, y)
int x, y;
{
    rect(x, y, TILE_W, TILE_H, COLOR_GREEN);
    rect(x + 4, y + 18, TILE_W - 8, 4, COLOR_BLACK);
    rect(x + 10, y + 8, 4, 4, COLOR_BLACK);
    rect(x + 23, y + 28, 4, 4, COLOR_BLACK);
}

river(x, y)
int x, y;
{
    rect(x, y, TILE_W, TILE_H, COLOR_BLUE);
    rect(x + 13, y, 9, TILE_H, COLOR_WHITE);
    rect(x + 16, y, 3, TILE_H, COLOR_BLUE);
    rect(x + 11, y + 12, 13, 4, COLOR_BLUE);
    rect(x + 11, y + 28, 13, 4, COLOR_BLUE);
}

mountn(x, y)
int x, y;
{
    rect(x, y, TILE_W, TILE_H, COLOR_GREEN);
    rect(x + 15, y + 4, 5, 7, COLOR_WHITE);
    rect(x + 11, y + 11, 13, 7, COLOR_WHITE);
    rect(x + 7, y + 18, 21, 9, COLOR_WHITE);
    rect(x + 3, y + 27, 29, 8, COLOR_WHITE);
    rect(x + 16, y + 5, 3, 9, COLOR_BLACK);
}

tile(kind, x, y)
int kind, x, y;
{
    if (kind == 1) {
        river(x, y);
    } else if (kind == 2) {
        mountn(x, y);
    } else {
        plain(x, y);
    }
}

mapdraw()
{
    int r, c;

    rect(0, 0, SCR_W, SCR_H, COLOR_BLACK);
    for (r = 0; r < MAP_ROWS; r++)
        for (c = 0; c < MAP_COLS; c++)
            tile(g_map[r][c],
                 MAP_OX + c * TILE_W,
                 MAP_OY + r * TILE_H);
}

player(x, y)
int x, y;
{
    rect(x + 6, y,      6, 4, COLOR_BLACK);
    rect(x + 4, y + 4, 10, 5, COLOR_BLACK);
    rect(x + 7, y + 9,  4, 3, COLOR_WHITE);
    rect(x + 2, y + 12,14, 7, COLOR_BLACK);
    rect(x + 3, y + 19, 4, 5, COLOR_WHITE);
    rect(x + 11,y + 19, 4, 5, COLOR_WHITE);
    rect(x + 7, y + 5,  4, 2, COLOR_WHITE);
}

mule(x, y)
int x, y;
{
    rect(x + 3, y + 3, 12, 8, COLOR_WHITE);
    rect(x,     y + 11,18, 6, COLOR_WHITE);
    rect(x + 1, y + 17, 4, 7, COLOR_BLACK);
    rect(x + 13,y + 17, 4, 7, COLOR_BLACK);
    rect(x + 12,y,      6, 4, COLOR_WHITE);
    rect(x + 14,y + 1,  2, 2, COLOR_BLACK);
}

render(sx1, sx2)
int sx1, sx2;
{
    int py, my;

    py = MAP_OY + 8;
    my = MAP_OY + TILE_H * 3;
    mapdraw();
    player(sx1, py);
    mule(sx2, my);
}

copyrows(y, h)
int y, h;
{
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y + h > SCR_H) h = SCR_H - y;
    if (h <= 0) return;
    memcpy(g_scr + y * SCR_BPR, g_back + y * SCR_BPR, h * SCR_BPR);
}

copytouch(y1, y2)
int y1, y2;
{
    int top, bot;

    top = y1;
    if (y2 < top) top = y2;
    bot = y1 + SPR_H;
    if (y2 + SPR_H > bot) bot = y2 + SPR_H;
    copyrows(top, bot - top);
}

animate()
{
    int frame, sx1, sx2;
    int py, my, start, end, elapsed;

    py = MAP_OY + 8;
    my = MAP_OY + TILE_H * 3;
    sx1 = MAP_OX;
    sx2 = MAP_OX + MAP_COLS * TILE_W - SPR_W;

    render(sx1, sx2);
    memcpy(g_scr, g_back, SCR_SIZE);
    nap(20);

    start = nowsec();
    printf("poc_tiles: full off-screen render, partial row copy\n");

    for (frame = 1; frame < 160; frame++) {
        sx1 = MAP_OX + (frame * 8) % (MAP_COLS * TILE_W - SPR_W);
        sx2 = MAP_OX + MAP_COLS * TILE_W - SPR_W
              - (frame * 8) % (MAP_COLS * TILE_W - SPR_W);

        render(sx1, sx2);
        copytouch(py, py);
        copytouch(my, my);
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0) {
            printf("poc_tiles: frames=%d seconds=%d fps=%d rows/frame=%d\n",
                   frame - 1, elapsed, (frame - 1) / elapsed, SPR_H * 2);
        } else {
            printf("poc_tiles: frames=%d seconds=<1 rows/frame=%d\n",
                   frame - 1, SPR_H * 2);
        }
    } else {
        printf("poc_tiles: F$Time unavailable\n");
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "poc_tiles: /w7 open failed\n");
        exit(1);
    }

    palinit();
    if (scinfo()) {
        close(g_win);
        exit(1);
    }

    if (mapwin()) {
        close(g_win);
        exit(1);
    }

    g_back = (unsigned char *)malloc(SCR_SIZE);
    if (!g_back) {
        fprintf(stderr, "poc_tiles: malloc failed\n");
        unmapwin();
        close(g_win);
        exit(1);
    }

    animate();

    unmapwin();
    free(g_back);
    close(g_win);
    exit(0);
}
