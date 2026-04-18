/***********************************************************************
 * poc_gfx.c  -  NitrOS-9 EOU / CoCo 3 GET/PUT graphics PoC
 *
 * Uses documented CoWin commands only:
 *   - DWSet creates a type 6 graphics window: 320x200, 4 colors
 *   - Palette sets visible test colors
 *   - FColor, SetDPtr, and Bar draw the initial background and sprite
 *   - GetBlk captures one 40x40 movement buffer per direction
 *   - /nil VRN VIRQ gives a documented 1/60 second timing gate
 *   - PutBlk moves and restores the sprite in one command per step
 *
 * Compile: dcc poc_gfx.c -s -m=8k -f=/dd/cmds/pocgfx
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define SCR_W     320
#define SCR_H     200
#define SCR_TYPE  6
#define SCR_COLS  40
#define SCR_ROWS  25
#define SPR_W     32
#define SPR_H     40
#define SPR_Y     80
#define STEP      8
#define TICKS     4
#define MOVE_W    40
#define GP_GRP    7
#define GP_RIGHT  1
#define GP_LEFT   2
#define VS_SIG    0x7f

#define COLOR_BLACK 0
#define COLOR_GREEN 1
#define COLOR_RED   2
#define COLOR_WHITE 3

#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif
#ifndef F_TIME
#define F_TIME    0x15
#endif
#ifndef I_SETSTT
#define I_SETSTT  0x8E
#endif
#ifndef SS_FSET
#define SS_FSET   0xC7
#endif

int open(), write(), close(), intercept();

static int g_win;
static int g_vpath;
static int g_vok;
static int g_vcnt;

vtrap(sig)
int sig;
{
    if (sig == VS_SIG) {
        g_vcnt++;
        return;
    }
    exit(sig);
}

wrwin(buf, len)
unsigned char *buf; int len;
{
    write(g_win, buf, len);
}

putwrd(buf, pos, value)
unsigned char *buf; int pos, value;
{
    buf[pos] = (unsigned char)((value >> 8) & 0xff);
    buf[pos + 1] = (unsigned char)(value & 0xff);
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

int vinit()
{
    struct registers r;

    g_vok = 0;
    g_vcnt = 0;
    g_vpath = open("/nil", 3);
    if (g_vpath < 0) {
        printf("poc_gfx: /nil open failed, VIRQ disabled\n");
        return -1;
    }

    intercept(vtrap);
    r.rg_a = (char)g_vpath;
    r.rg_b = (char)SS_FSET;
    r.rg_x = 1;
    r.rg_y = 1;
    r.rg_u = VS_SIG;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_gfx: SS.FSet VIRQ error #%d\n", r.rg_b & 0xff);
        close(g_vpath);
        g_vpath = -1;
        return -1;
    }

    g_vok = 1;
    printf("poc_gfx: /nil FS2+ VIRQ active, signal=%d\n", VS_SIG);
    return 0;
}

vwait()
{
    int next;
    struct registers r;

    if (!g_vok) {
        nap(2);
        return;
    }
    next = g_vcnt + 1;
    while (g_vcnt < next) {
        r.rg_x = 0;
        _os9(F_SLEEP, &r);
    }
}

vwaitn(count)
int count;
{
    int i;

    for (i = 0; i < count; i++) vwait();
}

vdone()
{
    if (g_vpath >= 0) close(g_vpath);
    g_vpath = -1;
    g_vok = 0;
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

fgset(prn)
int prn;
{
    unsigned char cmd[3];
    cmd[0] = 0x1b;
    cmd[1] = 0x32;
    cmd[2] = (unsigned char)prn;
    wrwin(cmd, 3);
}

dptr(x, y)
int x, y;
{
    unsigned char cmd[6];
    cmd[0] = 0x1b;
    cmd[1] = 0x40;
    putwrd(cmd, 2, x);
    putwrd(cmd, 4, y);
    wrwin(cmd, 6);
}

barabs(x, y)
int x, y;
{
    unsigned char cmd[6];
    cmd[0] = 0x1b;
    cmd[1] = 0x4a;
    putwrd(cmd, 2, x);
    putwrd(cmd, 4, y);
    wrwin(cmd, 6);
}

rect(x, y, w, h, color)
int x, y, w, h, color;
{
    if (w <= 0 || h <= 0) return;
    fgset(color);
    dptr(x, y);
    barabs(x + w - 1, y + h - 1);
}

getblk(grp, bfn, x, y, w, h)
int grp, bfn, x, y, w, h;
{
    unsigned char cmd[12];
    cmd[0] = 0x1b;
    cmd[1] = 0x2c;
    cmd[2] = (unsigned char)grp;
    cmd[3] = (unsigned char)bfn;
    putwrd(cmd, 4, x);
    putwrd(cmd, 6, y);
    putwrd(cmd, 8, w);
    putwrd(cmd, 10, h);
    wrwin(cmd, 12);
}

putblk(grp, bfn, x, y)
int grp, bfn, x, y;
{
    unsigned char cmd[8];
    cmd[0] = 0x1b;
    cmd[1] = 0x2d;
    cmd[2] = (unsigned char)grp;
    cmd[3] = (unsigned char)bfn;
    putwrd(cmd, 4, x);
    putwrd(cmd, 6, y);
    wrwin(cmd, 8);
}

kilbuf(grp, bfn)
int grp, bfn;
{
    unsigned char cmd[4];
    cmd[0] = 0x1b;
    cmd[1] = 0x2a;
    cmd[2] = (unsigned char)grp;
    cmd[3] = (unsigned char)bfn;
    wrwin(cmd, 4);
}

palinit()
{
    palset(COLOR_BLACK, 0x00);
    palset(COLOR_GREEN, 0x12);
    palset(COLOR_RED,   0x24);
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

bg_band(y, h)
int y, h;
{
    rect(0, y, SCR_W, h, ((y / 50) & 1) ? COLOR_RED : COLOR_GREEN);
}

bgdraw()
{
    bg_band(0, 50);
    bg_band(50, 50);
    bg_band(100, 50);
    bg_band(150, 50);
}

sprite_draw(x, y)
int x, y;
{
    rect(x, y, SPR_W, SPR_H, COLOR_WHITE);
    rect(x + 4, y + 8, SPR_W - 8, SPR_H - 16, COLOR_BLACK);
}

buffers_make()
{
    bgdraw();
    sprite_draw(STEP, SPR_Y);
    getblk(GP_GRP, GP_RIGHT, 0, SPR_Y, MOVE_W, SPR_H);

    bgdraw();
    sprite_draw(0, SPR_Y);
    getblk(GP_GRP, GP_LEFT, 0, SPR_Y, MOVE_W, SPR_H);

    bgdraw();
}

animate()
{
    int frame, x, dir, start, end, elapsed, ticks;

    x = 0;
    dir = 1;
    sprite_draw(x, SPR_Y);
    vwaitn(TICKS);

    start = nowsec();
    ticks = g_vcnt;
    printf("poc_gfx: union GET/PUT bounce, step=%d wait=%d ticks\n",
           STEP, TICKS);

    for (frame = 1; frame < 240; frame++) {
        if (dir > 0) {
            vwaitn(TICKS);
            putblk(GP_GRP, GP_RIGHT, x, SPR_Y);
            x += STEP;
            if (x >= SCR_W - SPR_W) dir = -1;
        } else {
            vwaitn(TICKS);
            putblk(GP_GRP, GP_LEFT, x - STEP, SPR_Y);
            x -= STEP;
            if (x <= 0) dir = 1;
        }
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0) {
            printf("poc_gfx: frames=%d seconds=%d fps=%d\n",
                   frame - 1, elapsed, (frame - 1) / elapsed);
            printf("poc_gfx: virq_ticks=%d tickrate=%d\n",
                   g_vcnt - ticks, (g_vcnt - ticks) / elapsed);
        } else {
            printf("poc_gfx: frames=%d seconds=<1\n", frame - 1);
        }
    } else {
        printf("poc_gfx: F$Time unavailable\n");
    }
}

main()
{
    g_vpath = -1;
    if (open_window()) {
        fprintf(stderr, "poc_gfx: /w7 open failed\n");
        exit(1);
    }

    palinit();
    kilbuf(GP_GRP, 0);
    buffers_make();
    vinit();
    animate();
    vdone();
    kilbuf(GP_GRP, 0);
    close(g_win);
    exit(0);
}
