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
#define NFRAMES    600

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
        c = ((y / 24) + frame) & 15;
        p = base + y * SCR_BPR;
        for (x = 0; x < SCR_BPR; x++) *p++ = (c << 4) | c;
    }

    bx = (frame * 4) % (SCR_W - 28);
    rect(base, bx, 70, 28, 32, 14);
    rect(base, bx + 5, 78, 18, 16, 1);
}

animate()
{
    int frame, start, end, elapsed;

    render(g_scr1, 0);
    render(g_scr2, 1);
    show_screen(g_num1);
    printf("poc_cvdg16: displayed initial frame on %s screen %d\n",
           g_devname, g_num1);
    nap(2);

    start = nowsec();
    printf("poc_cvdg16: flip-only speed test\n");
    for (frame = 0; frame < NFRAMES; frame++) {
        if (frame & 1)
            show_screen(g_num2);
        else
            show_screen(g_num1);
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0)
            printf("poc_cvdg16: flip frames=%d seconds=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        else
            printf("poc_cvdg16: flip frames=%d seconds=<1\n", frame);
    }

    start = nowsec();
    printf("poc_cvdg16: render+flip speed test\n");
    for (frame = 0; frame < NFRAMES; frame++) {
        if (frame & 1) {
            render(g_scr2, frame);
            show_screen(g_num2);
        } else {
            render(g_scr1, frame);
            show_screen(g_num1);
        }
    }

    end = nowsec();
    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0)
            printf("poc_cvdg16: render frames=%d seconds=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        else
            printf("poc_cvdg16: render frames=%d seconds=<1\n", frame);
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
