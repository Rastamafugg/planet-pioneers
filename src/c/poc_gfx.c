/***********************************************************************
 * poc_gfx.c  -  NitrOS-9 EOU / CoCo 3 CoWin Graphics Control PoC
 *
 * Verifies documented CoWin graphics commands without direct screen RAM:
 *   - DWSet creates a type 6 graphics window: 320x200, 4 colors
 *   - Palette assigns RGB values to palette registers
 *   - FColor selects the active drawing palette register
 *   - SetDPtr + Bar draw filled rectangles through CoWin
 *
 * Expected output:
 *   - Four horizontal bands: green, blue, green, blue
 *   - A white rectangle with a black inset moving left to right
 *
 * Compile: dcc poc_gfx.c -s -m=24k -f=/dd/cmds/pocgfx
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

/* Screen geometry: type 6, 320x200, 4 colors */
#define SCR_W     320
#define SCR_H     200
#define SCR_TYPE  6
#define SCR_COLS  40
#define SCR_ROWS  25

#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif

int open(), write(), close();

static int g_win;

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

selwin()
{
    unsigned char cmd[2];
    cmd[0] = 0x1b;
    cmd[1] = 0x21;
    wrwin(cmd, 2);
}

int open_window()
{
    unsigned char cmd[10];

    g_win = open("/w1", 3);
    if (g_win < 0) return -1;

    /* DWSet: ESC $20 STY CPX CPY SZX SZY PRN_FG PRN_BG PRN_BORDER */
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
    return 0;
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

palinit()
{
    palset(0, 0x00);   /* black */
    palset(1, 0x12);   /* green */
    palset(2, 0x09);   /* blue */
    palset(3, 0x3f);   /* white */
}

bgdraw()
{
    rect(0,   0, SCR_W, 50, 1);
    rect(0,  50, SCR_W, 50, 2);
    rect(0, 100, SCR_W, 50, 1);
    rect(0, 150, SCR_W, 50, 2);
}

animate()
{
    int frame, bx;

    for (frame = 0; frame < 180; frame++) {
        bx = (frame * 2) % (SCR_W - 32);

        bgdraw();
        rect(bx, 80, 32, 40, 3);
        rect(bx + 4, 88, 24, 24, 0);

        nap(1);
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "poc_gfx: /w1 open failed\n");
        exit(1);
    }

    palinit();
    bgdraw();
    animate();

    close(g_win);
    exit(0);
}
