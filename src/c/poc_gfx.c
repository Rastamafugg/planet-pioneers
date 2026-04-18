/***********************************************************************
 * poc_gfx.c  -  NitrOS-9 EOU / CoCo 3 direct graphics PoC
 *
 * Uses SS.ScInf + F$MapBlk after diagnostics to map a type 6
 * 320x200 4-color screen. Renders each frame into an off-screen
 * malloc buffer, then copies that complete frame to screen RAM.
 *
 * Expected SS.ScInf for this full-screen type 6 test:
 *   nb=2, off=0, cells x=0 w=40 y=0 h=25
 *
 * Compile: dcc poc_gfx.c -s -m=8k -f=/dd/cmds/pocgfx
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
    palset(0, 0x00);
    palset(1, 0x12);
    palset(2, 0x09);
    palset(3, 0x3f);
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
    cmd[7] = 3;
    cmd[8] = 0;
    cmd[9] = 0;
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
        printf("poc_gfx: SS.ScInf error #%d\n", r.rg_b & 0xff);
        return -1;
    }

    g_nblks = r.rg_a & 0xff;
    g_sblk = r.rg_b & 0xff;
    g_off = r.rg_x;
    wx = (r.rg_y >> 8) & 0xff;
    ww = r.rg_y & 0xff;
    wy = (r.rg_u >> 8) & 0xff;
    wh = r.rg_u & 0xff;

    printf("poc_gfx: SS.ScInf nb=%d sb=%d off=%x\n",
           g_nblks, g_sblk, g_off);
    printf("poc_gfx: cells x=%d w=%d y=%d h=%d\n",
           wx, ww, wy, wh);

    if (g_nblks != 2 || g_off != 0 || wx != 0 || ww != 40 ||
        wy != 0 || wh != 25) {
        printf("poc_gfx: unsupported screen mapping\n");
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
        printf("poc_gfx: F$MapBlk error #%d\n", r.rg_b & 0xff);
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

rect(x, y, w, h, c)
int x, y, w, h, c;
{
    int r;
    for (r = 0; r < h; r++) hline(x, y + r, w, c);
}

bgdraw()
{
    rect(0,   0, SCR_W, 50, 1);
    rect(0,  50, SCR_W, 50, 2);
    rect(0, 100, SCR_W, 50, 1);
    rect(0, 150, SCR_W, 50, 2);
}

render(bx)
int bx;
{
    bgdraw();
    rect(bx, 80, 32, 40, 3);
    rect(bx + 4, 88, 24, 24, 0);
}

flip()
{
    memcpy(g_scr, g_back, SCR_SIZE);
}

animate()
{
    int frame, bx, start, end, elapsed;

    start = nowsec();
    printf("poc_gfx: full-frame copy, sleep=off, step=8\n");

    for (frame = 0; frame < 240; frame++) {
        bx = (frame * 8) % (SCR_W - 32);
        render(bx);
        flip();
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0) {
            printf("poc_gfx: frames=%d seconds=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        } else {
            printf("poc_gfx: frames=%d seconds=<1\n", frame);
        }
    } else {
        printf("poc_gfx: F$Time unavailable\n");
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "poc_gfx: /w7 open failed\n");
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
        fprintf(stderr, "poc_gfx: malloc failed\n");
        unmapwin();
        close(g_win);
        exit(1);
    }

    memset(g_back, 0, SCR_SIZE);
    flip();
    animate();

    unmapwin();
    free(g_back);
    close(g_win);
    exit(0);
}
