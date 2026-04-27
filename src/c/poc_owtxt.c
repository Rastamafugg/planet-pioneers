/***********************************************************************
 * poc_owtxt.c - NitrOS-9 Phase 6.5 PoC: text/overlay over a CoVDG screen.
 *
 * Resolves three open questions from wiki/platform/cowin-text.md before
 * Phase 7a HUD/score code is written:
 *   Q1. Does OWSet (ESC $22) accept the path the render child uses for
 *       a CoVDG-allocated screen, or do we need a parallel CoWin device
 *       window (DWSet STY=$00 / $FF) on the same screen?
 *   Q2. Does the Palette/DefColr escape (ESC $31 / ESC $30) repaint
 *       previously-drawn pixels on a CoVDG-displayed screen?  (If so it
 *       is the simpler replacement for the deferred R_OP_PALETTE
 *       SS.PalSet codepath in render.c.)
 *   Q3. Is the standard font preloaded under `pioneer`, or do we need an
 *       explicit GPLoad + Font at startup?
 *
 * Standalone -- no shmem, no fork.  Mirrors poc_cvdg16's screen
 * allocation pattern.  All three probes share a single allocated screen.
 *
 * The probes are visual.  The user runs pocowtxt on EOU and reports back
 * what was on screen at each pause.  The printf transcript becomes
 * visible after SS.FScrn returns the user to the original terminal.
 *
 * Compile: dcc poc_owtxt.c -m=4k -f=/dd/cmds/pocowtxt
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define SCR_W      160
#define SCR_H      192
#define SCR_BPR    80
#define SCR_TYPE   2

#define COLOR_BLACK 0
#define COLOR_GREEN 1
#define COLOR_BLUE  2
#define COLOR_WHITE 3
#define COLOR_RED   4
#define COLOR_YEL   5

/* Palette register colour bytes (6-bit RGB 00rRgGbB), per cowin-text.md */
#define PAL_RED     0x24
#define PAL_GREEN   0x12
#define PAL_BLUE    0x09
#define PAL_WHITE   0x3f
#define PAL_BLACK   0x00

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

static int g_path;
static int g_wpath;             /* parallel CoWin path, -1 if unused */
static unsigned char *g_scr;
static int g_num;
static char *g_devname;

static wpath(buf, len)
unsigned char *buf; int len;
{
    write(g_path, buf, len);
}

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

static int alloc_screen()
{
    struct registers r;

    r.rg_a = (char)g_path;
    r.rg_b = (char)SS_ASCRN;
    r.rg_x = (unsigned)SCR_TYPE;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_owtxt: %s SS.AScrn err #%d\n",
               g_devname, r.rg_b & 0xff);
        return -1;
    }
    g_scr = (unsigned char *)r.rg_x;
    g_num = r.rg_y;
    printf("poc_owtxt: %s screen %d at %x\n",
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
        printf("poc_owtxt: SS.DScrn %d err #%d\n", num, r.rg_b & 0xff);
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
    wpath(cmd, 2);
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
        if (g_path < 0) {
            printf("poc_owtxt: %s open failed\n", g_devname);
            continue;
        }
        if (alloc_screen() == 0) return 0;
        close(g_path);
    }
    g_path = -1;
    g_devname = "";
    return -1;
}

/* ------------------------------------------------------------------ */
/* Q1a: OWSet directly on the CoVDG path                              */
/* ------------------------------------------------------------------ */
static q1a_owset()
{
    unsigned char cmd[8];

    /* ESC $22  SVS CPX CPY SZX SZY PRN1 PRN2
       SVS=0 (no save -- we don't care about restore for the probe).
       Coords in standard chars.  Standard char on a 160x192 screen is
       8x8, giving a 20x24 char grid.  Place box near the top so it
       does not overlap the Q2 colour bars in the lower half.        */
    cmd[0] = 0x1b; cmd[1] = 0x22;
    cmd[2] = 0x00;          /* SVS = no save */
    cmd[3] = 0x01;          /* CPX char col 1 */
    cmd[4] = 0x01;          /* CPY char row 1 */
    cmd[5] = 0x12;          /* SZX 18 chars */
    cmd[6] = 0x04;          /* SZY 4 chars  */
    cmd[7] = 0x0f;          /* PRN1 fg pal reg 15 */
    wpath(cmd, 8);
    cmd[0] = 0x00;          /* PRN2 bg pal reg 0 */
    wpath(cmd, 1);

    /* Q3 probe (pre-GPLoad) is interleaved here -- if letters render
       inside the OWSet box, the standard font is preloaded.          */
    wpath((unsigned char *)"OWSET-OK abc123", 15);
}

/* ------------------------------------------------------------------ */
/* Q1b: parallel CoWin device window on the same screen.              */
/* Try /w with DWSet STY=$00 (process's current screen), then $FF.    */
/* ------------------------------------------------------------------ */
static int q1b_parallel()
{
    unsigned char cmd[10];
    int p;

    p = open("/w", 3);
    if (p < 0) {
        printf("poc_owtxt: /w open failed\n");
        return -1;
    }

    /* ESC $20 STY CPX CPY SZX SZY PRN1 PRN2 PRN3 */
    cmd[0] = 0x1b; cmd[1] = 0x20;
    cmd[2] = 0x00;          /* STY = process's current screen */
    cmd[3] = 0x01; cmd[4] = 0x10;   /* CPX=1 CPY=16 (lower half free) */
    cmd[5] = 0x12; cmd[6] = 0x04;   /* SZX=18 SZY=4 */
    cmd[7] = 0x0f; cmd[8] = 0x00; cmd[9] = 0x00;
    write(p, cmd, 10);
    write(p, "PARALLEL-OK abc", 15);
    g_wpath = p;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Q2: Palette / DefColr escapes on the displayed CoVDG screen.       */
/* The reference bars were drawn into pal regs 4 and 5 below.         */
/* If the escape works, the bars repaint when we change those regs.   */
/* ------------------------------------------------------------------ */
static q2_palette(p)
int p;
{
    unsigned char cmd[4];

    /* Re-point reg 4 (currently red) to blue. */
    cmd[0] = 0x1b; cmd[1] = 0x31;
    cmd[2] = 0x04;          /* PRN */
    cmd[3] = PAL_BLUE;      /* CTN */
    write(p, cmd, 4);

    /* Re-point reg 5 (currently yellow) to white. */
    cmd[2] = 0x05;
    cmd[3] = PAL_WHITE;
    write(p, cmd, 4);
}

static q2_defcolr(p)
int p;
{
    unsigned char cmd[2];
    cmd[0] = 0x1b; cmd[1] = 0x30;
    write(p, cmd, 2);
}

/* ------------------------------------------------------------------ */
/* Q3 fallback: explicit GPLoad + Font if pre-GPLoad letters were `.` */
/* GPLoad: ESC $39 fontname<CR>     Font: ESC $3A fontnum             */
/* ------------------------------------------------------------------ */
static q3_loadfont(p)
int p;
{
    unsigned char cmd[2];

    cmd[0] = 0x1b; cmd[1] = 0x39;
    write(p, cmd, 2);
    write(p, "stdfont\015", 8);   /* \015 = CR; OS-9 EOL */

    cmd[0] = 0x1b; cmd[1] = 0x3a;
    write(p, cmd, 2);
    cmd[0] = 0x00;                /* font num 0 */
    write(p, cmd, 1);

    write(p, " post-GPLoad: abc123", 20);
}

main()
{
    g_wpath = -1;

    if (open_vdg() < 0) {
        printf("poc_owtxt: no VDG path accepted SS.AScrn\n");
        exit(1);
    }
    vsel();
    if (show_screen(g_num) < 0) {
        free_screen(g_num);
        close(g_path);
        exit(1);
    }
    nap(30);   /* ~0.5s settle */

    /* Reference colour bars in palette registers 4 and 5.
       Direct byte writes -- no escape sequences yet.                */
    hbar(g_scr, 0,   80, 160, 16, COLOR_RED);   /* uses pal reg 4 */
    hbar(g_scr, 0,  100, 160, 16, COLOR_YEL);   /* uses pal reg 5 */
    nap(60);

    /* Q1a: OWSet on the CoVDG path.  Q3 pre-GPLoad probe rides along
       in the same string ("OWSET-OK abc123").                       */
    q1a_owset();
    nap(180);   /* 3s for user to read */

    /* Q1b: parallel CoWin window.  Run regardless of Q1a result --
       both data points are useful and the parallel window lands in
       the lower half so it doesn't fight the Q1a box.               */
    if (q1b_parallel() == 0) nap(180);

    /* Q2: palette repaint.  Drive through whichever text path Q1
       produced; prefer the parallel one because we know it has a
       valid DWSet state.  If Q1b failed, try the CoVDG path.        */
    if (g_wpath >= 0) q2_palette(g_wpath);
    else              q2_palette(g_path);
    nap(180);

    if (g_wpath >= 0) q2_defcolr(g_wpath);
    else              q2_defcolr(g_path);
    nap(120);

    /* Q3 fallback: explicit GPLoad + Font.  Only needed if pre-GPLoad
       letters rendered as `.`; we run it unconditionally so the user
       can compare the two strings on screen.                        */
    if (g_wpath >= 0) q3_loadfont(g_wpath);
    else              q3_loadfont(g_path);
    nap(240);   /* 4s final read */

    /* Cleanup */
    if (g_wpath >= 0) close(g_wpath);
    show_screen(0);
    free_screen(g_num);
    close(g_path);

    printf("poc_owtxt: done.  Report on-screen results:\n");
    printf("  Q1a: did 'OWSET-OK abc123' appear in upper box?\n");
    printf("  Q1b: did 'PARALLEL-OK abc' appear in lower box?\n");
    printf("  Q2:  did the red bar turn blue and yellow bar turn white?\n");
    printf("       did DefColr revert them?\n");
    printf("  Q3:  pre-GPLoad letters readable, or `.`?\n");
    printf("       post-GPLoad ' post-GPLoad: abc123' readable?\n");
    exit(0);
}
