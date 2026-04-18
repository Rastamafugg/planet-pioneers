/***********************************************************************
 * poc_cvdg.c - NitrOS-9 CoVDG 320x192x16 page-flip PoC
 *
 * Uses documented CoVDG SS.AScrn / SS.DScrn / SS.FScrn calls.
 * Screen type 4 is 320x192x16 colors and occupies 32K.
 *
 * Compile: dcc poc_cvdg.c -s -m=4k -f=/dd/cmds/poccvdg
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

#define SCR_W      320
#define SCR_H      192
#define SCR_BPR    160
#define SCR_TYPE   4
#define NFRAMES    80

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

int open(), close();

static int g_path;
static unsigned char *g_scr1;
static unsigned char *g_scr2;
static int g_num1;
static int g_num2;

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
        printf("poc_cvdg: SS.AScrn error #%d\n", r.rg_b & 0xff);
        return -1;
    }

    *pp = (unsigned char *)r.rg_x;
    *np = r.rg_y;
    printf("poc_cvdg: screen %d at %x\n", *np, (unsigned)*pp);
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
        printf("poc_cvdg: SS.DScrn %d error #%d\n", num, r.rg_b & 0xff);
        return -1;
    }
    return 0;
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

render(base, frame)
unsigned char *base; int frame;
{
    int y, x, c, bx;
    unsigned char *p;

    for (y = 0; y < SCR_H; y++) {
        c = (y / 24) & 15;
        p = base + y * SCR_BPR;
        for (x = 0; x < SCR_BPR; x++) *p++ = (c << 4) | c;
    }

    bx = (frame * 8) % (SCR_W - 40);
    rect(base, bx, 70, 40, 32, 14);
    rect(base, bx + 6, 78, 28, 16, 1);
}

animate()
{
    int frame, start, end, elapsed;

    start = nowsec();
    printf("poc_cvdg: page flip, 320x192x16\n");

    for (frame = 0; frame < NFRAMES; frame++) {
        if (frame & 1) {
            render(g_scr2, frame);
            show_screen(g_num2);
        } else {
            render(g_scr1, frame);
            show_screen(g_num1);
        }
        nap(2);
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0)
            printf("poc_cvdg: frames=%d seconds=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        else
            printf("poc_cvdg: frames=%d seconds=<1\n", frame);
    }
}

main()
{
    g_path = open("/term", 3);
    if (g_path < 0) {
        fprintf(stderr, "poc_cvdg: /term open failed\n");
        exit(1);
    }

    printf("poc_cvdg: allocating two 32K screens\n");
    if (alloc_screen(&g_scr1, &g_num1)) {
        close(g_path);
        exit(1);
    }
    if (alloc_screen(&g_scr2, &g_num2)) {
        free_screen(g_num1);
        close(g_path);
        exit(1);
    }

    animate();
    show_screen(0);
    free_screen(g_num1);
    free_screen(g_num2);
    close(g_path);
    exit(0);
}
