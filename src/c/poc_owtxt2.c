/***********************************************************************
 * poc_owtxt2.c - Phase 6.5 follow-up: three tighter probes after v1's
 * negative result on all six on-screen prompts.
 *
 *   P1  Fixed Q1b: open /w1 (a real numbered window descriptor) and
 *       DWSet STY=$FF (currently displayed screen).  v1 mistakenly
 *       opened the parent /w descriptor without iniz, so this is the
 *       actual answer to "does a parallel CoWin device window land
 *       on top of a CoVDG-displayed screen?".
 *
 *   P2  Q2 in isolation: send ESC $31 PRN CTN directly to the CoVDG
 *       VDG path itself.  Tech Ref shows there is no SetStat function
 *       code for setting an individual palette register -- only SS.Palet
 *       ($91) GET and SS.DFPal ($97) SET-default exist.  The ESC $31
 *       Palette escape on the window driver is the only documented
 *       per-register set; this probe asks whether it is honoured by the
 *       CoVDG path that already accepts ESC $21 Select.
 *
 *   P3  Font-glyph demo: render "HELLO" with a bit-packed 8x8 bitmap
 *       font drawn directly into the CoVDG screen via putpx.  Confirms
 *       the option-2 fallback (render-child-owned glyphs, no CoWin
 *       overlay) is feasible for Phase 7a HUD/score.
 *
 * Compile: dcc poc_owtxt2.c -m=4k -f=/dd/cmds/pocowtx2
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define SCR_W      160
#define SCR_H      192
#define SCR_BPR    80
#define SCR_TYPE   2

#define COLOR_BLACK 0
#define COLOR_RED   4
#define COLOR_YEL   5
#define COLOR_WHITE 15

/* Palette colour bytes (6-bit RGB 00rRgGbB) */
#define PAL_BLUE    0x09
#define PAL_WHITE   0x3f

#ifndef I_SETSTT
#define I_SETSTT   0x8E
#endif
#ifndef F_SLEEP
#define F_SLEEP    0x0A
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

static int g_path;          /* CoVDG VDG path */
static int g_wpath;         /* /wN parallel window path, -1 if unused */
static unsigned char *g_scr;
static int g_num;
static char *g_devname;

/* --- bit-packed 8x8 font: H E L L O (5 glyphs, 8 bytes each) -------- */
static unsigned char g_font[5][8] = {
    /* H */ { 0x42,0x42,0x42,0x7e,0x42,0x42,0x42,0x00 },
    /* E */ { 0x7e,0x40,0x40,0x7c,0x40,0x40,0x7e,0x00 },
    /* L */ { 0x40,0x40,0x40,0x40,0x40,0x40,0x7e,0x00 },
    /* L */ { 0x40,0x40,0x40,0x40,0x40,0x40,0x7e,0x00 },
    /* O */ { 0x3c,0x42,0x42,0x42,0x42,0x42,0x3c,0x00 }
};

static nap(ticks)
int ticks;
{
    struct registers r;
    r.rg_x = (unsigned)ticks;
    _os9(F_SLEEP, &r);
}

static putpx(base, x, y, c)
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

static hbar(base, x, y, w, h, c)
unsigned char *base; int x, y, w, h, c;
{
    int r, i;
    for (r = 0; r < h; r++)
        for (i = 0; i < w; i++) putpx(base, x + i, y + r, c);
}

static draw_glyph(base, gx, gy, glyph, c)
unsigned char *base; int gx, gy; unsigned char *glyph; int c;
{
    int row, col;
    unsigned char b;

    for (row = 0; row < 8; row++) {
        b = glyph[row];
        for (col = 0; col < 8; col++) {
            if (b & (0x80 >> col))
                putpx(base, gx + col, gy + row, c);
        }
    }
}

static draw_hello(base, gx, gy, c)
unsigned char *base; int gx, gy, c;
{
    int i;
    for (i = 0; i < 5; i++)
        draw_glyph(base, gx + i * 9, gy, g_font[i], c);
}

static int alloc_screen()
{
    struct registers r;

    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_ASCRN;
    r.rg_x = (unsigned)SCR_TYPE;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_owtxt2: %s SS.AScrn err #%d\n",
               g_devname, r.rg_b & 0xff);
        return -1;
    }
    g_scr = (unsigned char *)r.rg_x;
    g_num = r.rg_y;
    printf("poc_owtxt2: %s screen %d at %x\n",
           g_devname, g_num, (unsigned)g_scr);
    return 0;
}

static int show_screen(num)
int num;
{
    struct registers r;
    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_DSCRN;
    r.rg_y = (unsigned)num;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_owtxt2: SS.DScrn %d err #%d\n", num, r.rg_b & 0xff);
        return -1;
    }
    return 0;
}

static free_screen(num)
int num;
{
    struct registers r;
    if (num <= 0) return;
    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_FSCRN;
    r.rg_y = (unsigned)num;
    _os9(I_SETSTT, &r);
}

static vsel()
{
    unsigned char cmd[2];
    cmd[0] = 0x1b; cmd[1] = 0x21;
    write(g_path, cmd, 2);
    nap(2);
}

static int open_vdg()
{
    char *names[6];
    int i;

    names[0] = "/verm";
    names[1] = "/VERM";
    names[2] = "/term_vdg";
    names[3] = "/TERM_VDG";
    names[4] = "/term";
    names[5] = 0;

    for (i = 0; names[i]; i++) {
        g_devname = names[i];
        g_path = open(g_devname, 3);
        if (g_path < 0) continue;
        if (alloc_screen() == 0) return 0;
        close(g_path);
    }
    g_path = -1;
    return -1;
}

/* ------------------------------------------------------------------ */
/* P1: open /wN, DWSet STY=$FF, write a banner.                       */
/* ------------------------------------------------------------------ */
static int p1_parallel()
{
    char *names[5];
    unsigned char cmd[10];
    int i, p;

    names[0] = "/w1";
    names[1] = "/w2";
    names[2] = "/w7";
    names[3] = "/w8";
    names[4] = 0;

    for (i = 0; names[i]; i++) {
        p = open(names[i], 3);
        if (p < 0) {
            printf("poc_owtxt2: %s open failed\n", names[i]);
            continue;
        }
        printf("poc_owtxt2: %s opened, path %d\n", names[i], p);

        /* ESC $20 STY CPX CPY SZX SZY PRN1 PRN2 PRN3 */
        cmd[0] = 0x1b; cmd[1] = 0x20;
        cmd[2] = 0xff;          /* STY = currently displayed screen */
        cmd[3] = 0x01; cmd[4] = 0x01;
        cmd[5] = 0x12; cmd[6] = 0x04;
        cmd[7] = 0x0f; cmd[8] = 0x00; cmd[9] = 0x00;
        write(p, cmd, 10);
        write(p, "PARALLEL-FIXED ", 15);
        write(p, names[i], 3);
        g_wpath = p;
        return 0;
    }
    g_wpath = -1;
    return -1;
}

/* ------------------------------------------------------------------ */
/* P2: ESC $31 PRN CTN sent to the CoVDG VDG path itself.             */
/* If the bars were drawn in palette regs 4 (red) and 5 (yellow),     */
/* re-pointing reg 4 -> blue and reg 5 -> white repaints them.        */
/* ------------------------------------------------------------------ */
static p2_palette_via_vdg()
{
    unsigned char cmd[4];

    cmd[0] = 0x1b; cmd[1] = 0x31;
    cmd[2] = 0x04; cmd[3] = PAL_BLUE;
    write(g_path, cmd, 4);

    cmd[2] = 0x05; cmd[3] = PAL_WHITE;
    write(g_path, cmd, 4);
}

main()
{
    g_wpath = -1;

    if (open_vdg() < 0) {
        printf("poc_owtxt2: no VDG path accepted SS.AScrn\n");
        exit(1);
    }
    vsel();
    if (show_screen(g_num) < 0) {
        free_screen(g_num);
        close(g_path);
        exit(1);
    }
    nap(30);

    /* Reference colour bars in palette regs 4 (red) and 5 (yellow). */
    hbar(g_scr, 0,  80, 160, 16, COLOR_RED);
    hbar(g_scr, 0, 100, 160, 16, COLOR_YEL);
    nap(60);

    /* P3: render HELLO via direct putpx into the CoVDG screen.       */
    draw_hello(g_scr, 8, 40, COLOR_WHITE);
    nap(180);

    /* P1: parallel CoWin window (fixed: /wN with iniz, STY=$FF).     */
    p1_parallel();
    nap(180);

    /* P2: palette change via ESC $31 to the CoVDG path itself.       */
    p2_palette_via_vdg();
    nap(240);

    if (g_wpath >= 0) close(g_wpath);
    show_screen(0);
    free_screen(g_num);
    close(g_path);

    printf("poc_owtxt2: done.  Report on-screen results:\n");
    printf("  P1: did 'PARALLEL-FIXED /wN' appear in upper box?\n");
    printf("  P2: did the red bar turn blue and yellow bar turn white?\n");
    printf("  P3: did 'HELLO' appear in white above the bars?\n");
    exit(0);
}
