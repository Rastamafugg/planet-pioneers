/***********************************************************************
 * poc_cwext.c - NitrOS-9 CoWin 320x200x16 external-buffer PoC
 *
 * Creates a CoWin type 8 screen, maps the visible 32K screen with
 * SS.ScInf/F$MapBlk, allocates a 32K external back buffer with SS.ARAM,
 * renders the back buffer one 8K block at a time, then copies each 8K
 * block to the visible screen.
 *
 * Compile: dcc poc_cwext.c -s -m=8k -f=/dd/cmds/poccwext
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

#define SCR_W      320
#define SCR_H      200
#define SCR_BPR    160
#define SCR_SIZE   32000
#define SCR_TYPE   8
#define SCR_COLS   40
#define SCR_ROWS   25
#define BLK_SIZE   8192
#define BLK_COUNT  4
#define NFRAMES    80

#ifndef F_SLEEP
#define F_SLEEP    0x0A
#endif
#ifndef F_TIME
#define F_TIME     0x15
#endif
#ifndef I_GETSTT
#define I_GETSTT   0x8D
#endif
#ifndef I_SETSTT
#define I_SETSTT   0x8E
#endif
#ifndef SS_SCINF
#define SS_SCINF   0x8F
#endif
#ifndef SS_ARAM
#define SS_ARAM    0xCA
#endif
#ifndef SS_DRAM
#define SS_DRAM    0xCB
#endif
#ifndef F_MAPBLK
#define F_MAPBLK   0x4F
#endif
#ifndef F_CLRBLK
#define F_CLRBLK   0x50
#endif

int open(), write(), close();

static int g_win;
static int g_nil;
static int g_nblks;
static int g_sblk;
static int g_off;
static int g_extblk;
static unsigned char *g_scr;
static unsigned char *g_ext;

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
    int i;

    for (i = 0; i < 16; i++) palset(i, (i * 4) & 63);
    palset(0, 0x00);
    palset(1, 0x09);
    palset(2, 0x12);
    palset(3, 0x1b);
    palset(14, 0x3f);
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
    cmd[7] = 14;
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
        printf("poc_cwext: SS.ScInf error #%d\n", r.rg_b & 0xff);
        return -1;
    }

    g_nblks = r.rg_a & 0xff;
    g_sblk = r.rg_b & 0xff;
    g_off = r.rg_x;
    wx = (r.rg_y >> 8) & 0xff;
    ww = r.rg_y & 0xff;
    wy = (r.rg_u >> 8) & 0xff;
    wh = r.rg_u & 0xff;

    printf("poc_cwext: SS.ScInf nb=%d sb=%d off=%x\n",
           g_nblks, g_sblk, g_off);
    printf("poc_cwext: cells x=%d w=%d y=%d h=%d\n", wx, ww, wy, wh);

    if (g_nblks != 4 || g_off != 0 || wx != 0 || ww != 40 ||
        wy != 0 || wh != 25) {
        printf("poc_cwext: unsupported screen mapping\n");
        return -1;
    }
    return 0;
}

int mapblocks(start, count, pp)
int start, count; unsigned char **pp;
{
    struct registers r;

    r.rg_b = (char)count;
    r.rg_x = (unsigned)start;
    if (_os9(F_MAPBLK, &r)) {
        printf("poc_cwext: F$MapBlk block=%d count=%d error #%d\n",
               start, count, r.rg_b & 0xff);
        return -1;
    }
    *pp = (unsigned char *)r.rg_u;
    return 0;
}

clrblocks(ptr, count)
unsigned char *ptr; int count;
{
    struct registers r;
    r.rg_b = (char)count;
    r.rg_u = (unsigned)ptr;
    _os9(F_CLRBLK, &r);
}

int mapscreen()
{
    if (mapblocks(g_sblk, g_nblks, &g_scr)) return -1;
    g_scr += g_off;
    return 0;
}

unmapscreen()
{
    if (g_scr) clrblocks(g_scr - g_off, g_nblks);
    g_scr = 0;
}

int alloc_ext()
{
    struct registers r;

    g_nil = open("/nil", 3);
    if (g_nil < 0) {
        printf("poc_cwext: /nil open failed\n");
        return -1;
    }

    r.rg_a = (char)g_nil;
    r.rg_b = (char)SS_ARAM;
    r.rg_x = (unsigned)BLK_COUNT;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_cwext: SS.ARAM error #%d\n", r.rg_b & 0xff);
        close(g_nil);
        g_nil = -1;
        return -1;
    }

    g_extblk = r.rg_x;
    printf("poc_cwext: external blocks start=%d count=%d\n",
           g_extblk, BLK_COUNT);
    return 0;
}

free_ext()
{
    struct registers r;

    if (g_nil < 0) return;
    r.rg_a = (char)g_nil;
    r.rg_b = (char)SS_DRAM;
    _os9(I_SETSTT, &r);
    close(g_nil);
    g_nil = -1;
}

putpx_chunk(chunk, x, y, c)
int chunk, x, y, c;
{
    int offs;
    unsigned char *p;

    if ((unsigned)x >= SCR_W || (unsigned)y >= SCR_H) return;
    offs = y * SCR_BPR + (x >> 1) - chunk * BLK_SIZE;
    if (offs < 0 || offs >= BLK_SIZE) return;
    p = g_ext + offs;
    if (x & 1)
        *p = (*p & 0xf0) | (c & 15);
    else
        *p = (*p & 0x0f) | ((c & 15) << 4);
}

hline_chunk(chunk, x, y, w, c)
int chunk, x, y, w, c;
{
    int i;

    if (w <= 0 || y < 0 || y >= SCR_H) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > SCR_W) w = SCR_W - x;
    for (i = 0; i < w; i++) putpx_chunk(chunk, x + i, y, c);
}

rect_chunk(chunk, x, y, w, h, c)
int chunk, x, y, w, h, c;
{
    int r;
    for (r = 0; r < h; r++) hline_chunk(chunk, x, y + r, w, c);
}

render_chunk(chunk, frame)
int chunk, frame;
{
    int i, y, color, global;
    int bx;

    for (i = 0; i < BLK_SIZE; i++) {
        global = chunk * BLK_SIZE + i;
        if (global < SCR_SIZE) {
            y = global / SCR_BPR;
            color = ((y / 25) + frame) & 15;
            g_ext[i] = (color << 4) | color;
        } else {
            g_ext[i] = 0;
        }
    }

    bx = (frame * 8) % (SCR_W - 40);
    rect_chunk(chunk, bx, 80, 40, 32, 14);
    rect_chunk(chunk, bx + 6, 88, 28, 16, 1);
}

int render_ext(frame)
int frame;
{
    int chunk;

    for (chunk = 0; chunk < BLK_COUNT; chunk++) {
        if (mapblocks(g_extblk + chunk, 1, &g_ext)) return -1;
        render_chunk(chunk, frame);
        clrblocks(g_ext, 1);
        g_ext = 0;
    }
    return 0;
}

int copy_ext()
{
    int chunk, bytes;

    for (chunk = 0; chunk < BLK_COUNT; chunk++) {
        if (mapblocks(g_extblk + chunk, 1, &g_ext)) return -1;
        bytes = BLK_SIZE;
        if (chunk == 3) bytes = SCR_SIZE - 3 * BLK_SIZE;
        memcpy(g_scr + chunk * BLK_SIZE, g_ext, bytes);
        clrblocks(g_ext, 1);
        g_ext = 0;
    }
    return 0;
}

animate()
{
    int frame, start, end, elapsed;

    start = nowsec();
    printf("poc_cwext: CoWin type 8, 32K external back buffer\n");

    for (frame = 0; frame < NFRAMES; frame++) {
        if (render_ext(frame)) return;
        if (copy_ext()) return;
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0)
            printf("poc_cwext: frames=%d seconds=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        else
            printf("poc_cwext: frames=%d seconds=<1\n", frame);
    }
}

main()
{
    g_nil = -1;
    if (open_window()) {
        fprintf(stderr, "poc_cwext: /w7 open failed\n");
        exit(1);
    }

    palinit();
    if (scinfo()) {
        close(g_win);
        exit(1);
    }
    if (mapscreen()) {
        close(g_win);
        exit(1);
    }
    if (alloc_ext()) {
        unmapscreen();
        close(g_win);
        exit(1);
    }

    animate();

    free_ext();
    unmapscreen();
    close(g_win);
    exit(0);
}
