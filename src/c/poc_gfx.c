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

write_cmd(buf, len)
unsigned char *buf; int len;
{
    write(g_win, buf, len);
}

put_word(buf, pos, value)
unsigned char *buf; int pos, value;
{
    buf[pos] = (unsigned char)((value >> 8) & 0xff);
    buf[pos + 1] = (unsigned char)(value & 0xff);
}

sleep_ticks(ticks)
int ticks;
{
    struct registers r;
    r.rg_x = (unsigned)ticks;
    _os9(F_SLEEP, &r);
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
    write_cmd(cmd, 10);
    sleep_ticks(2);
    return 0;
}

set_palette(prn, ctn)
int prn, ctn;
{
    unsigned char cmd[4];
    cmd[0] = 0x1b;
    cmd[1] = 0x31;
    cmd[2] = (unsigned char)prn;
    cmd[3] = (unsigned char)ctn;
    write_cmd(cmd, 4);
}

set_fcolor(prn)
int prn;
{
    unsigned char cmd[3];
    cmd[0] = 0x1b;
    cmd[1] = 0x32;
    cmd[2] = (unsigned char)prn;
    write_cmd(cmd, 3);
}

set_draw_ptr(x, y)
int x, y;
{
    unsigned char cmd[6];
    cmd[0] = 0x1b;
    cmd[1] = 0x40;
    put_word(cmd, 2, x);
    put_word(cmd, 4, y);
    write_cmd(cmd, 6);
}

bar_to(x, y)
int x, y;
{
    unsigned char cmd[6];
    cmd[0] = 0x1b;
    cmd[1] = 0x4a;
    put_word(cmd, 2, x);
    put_word(cmd, 4, y);
    write_cmd(cmd, 6);
}

fill_rect(x, y, w, h, color)
int x, y, w, h, color;
{
    if (w <= 0 || h <= 0) return;
    set_fcolor(color);
    set_draw_ptr(x, y);
    bar_to(x + w - 1, y + h - 1);
}

init_palette()
{
    set_palette(0, 0x00);   /* black */
    set_palette(1, 0x12);   /* green */
    set_palette(2, 0x09);   /* blue */
    set_palette(3, 0x3f);   /* white */
}

draw_background()
{
    fill_rect(0,   0, SCR_W, 50, 1);
    fill_rect(0,  50, SCR_W, 50, 2);
    fill_rect(0, 100, SCR_W, 50, 1);
    fill_rect(0, 150, SCR_W, 50, 2);
}

animate()
{
    int frame, bx;

    for (frame = 0; frame < 180; frame++) {
        bx = (frame * 2) % (SCR_W - 32);

        draw_background();
        fill_rect(bx, 80, 32, 40, 3);
        fill_rect(bx + 4, 88, 24, 24, 0);

        sleep_ticks(1);
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "poc_gfx: /w1 open failed\n");
        exit(1);
    }

    init_palette();
    draw_background();
    animate();

    close(g_win);
    exit(0);
}
