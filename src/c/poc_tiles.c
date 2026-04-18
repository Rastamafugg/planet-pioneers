/***********************************************************************
 * poc_tiles.c  -  NitrOS-9 EOU / CoCo 3 Tile & Sprite Control PoC
 *
 * Verifies tile-grid and sprite placement using documented CoWin output:
 *   - DWSet creates a type 6 graphics window: 320x200, 4 colors
 *   - Palette assigns black, green, blue, and white
 *   - FColor, SetDPtr, and Bar draw all tile and sprite rectangles
 *   - 9x5 map grid is centered on screen
 *   - Two animated marker sprites traverse the grid
 *
 * This version intentionally avoids SS.ScInf, F$MapBlk, and direct screen RAM.
 *
 * Compile: dcc poc_tiles.c -s -m=24k -f=/dd/cmds/poctiles
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define SCR_W     320
#define SCR_H     200
#define SCR_TYPE  6
#define SCR_COLS  40
#define SCR_ROWS  25

#define TILE_W    16
#define TILE_H    16
#define MAP_COLS  9
#define MAP_ROWS  5
#define MAP_OX    88
#define MAP_OY    60

#define COLOR_BLACK 0
#define COLOR_GREEN 1
#define COLOR_BLUE  2
#define COLOR_WHITE 3

#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif

int open(), write(), close();

static int g_win;

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
    palset(COLOR_BLACK, 0x00);
    palset(COLOR_GREEN, 0x12);
    palset(COLOR_BLUE,  0x09);
    palset(COLOR_WHITE, 0x3f);
}

cls()
{
    rect(0, 0, SCR_W, SCR_H, COLOR_BLACK);
}

plain(x, y)
int x, y;
{
    rect(x, y, TILE_W, TILE_H, COLOR_GREEN);
    rect(x + 2, y + 7, 12, 2, COLOR_BLACK);
}

river(x, y)
int x, y;
{
    rect(x, y, TILE_W, TILE_H, COLOR_BLUE);
    rect(x + 6, y, 4, TILE_H, COLOR_WHITE);
    rect(x + 7, y, 2, TILE_H, COLOR_BLUE);
}

mountn(x, y)
int x, y;
{
    rect(x, y, TILE_W, TILE_H, COLOR_GREEN);
    rect(x + 6, y + 2, 4, 3, COLOR_WHITE);
    rect(x + 4, y + 5, 8, 3, COLOR_WHITE);
    rect(x + 2, y + 8, 12, 4, COLOR_WHITE);
    rect(x + 7, y + 2, 2, 3, COLOR_BLACK);
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

    for (r = 0; r < MAP_ROWS; r++)
        for (c = 0; c < MAP_COLS; c++)
            tile(g_map[r][c],
                 MAP_OX + c * TILE_W,
                 MAP_OY + r * TILE_H);
}

player(x, y)
int x, y;
{
    rect(x + 2, y,     4, 2, COLOR_WHITE);
    rect(x + 1, y + 2, 6, 2, COLOR_WHITE);
    rect(x + 3, y + 4, 2, 1, COLOR_WHITE);
    rect(x,     y + 5, 8, 2, COLOR_WHITE);
    rect(x + 1, y + 7, 2, 1, COLOR_WHITE);
    rect(x + 5, y + 7, 2, 1, COLOR_WHITE);
}

mule(x, y)
int x, y;
{
    rect(x + 1, y + 1, 6, 3, COLOR_GREEN);
    rect(x,     y + 4, 8, 2, COLOR_GREEN);
    rect(x,     y + 6, 2, 2, COLOR_GREEN);
    rect(x + 6, y + 6, 2, 2, COLOR_GREEN);
    rect(x + 5, y,     3, 1, COLOR_WHITE);
}

frame(player_x, mule_x)
int player_x, mule_x;
{
    cls();
    mapdraw();
    player(player_x, MAP_OY + 4);
    mule(mule_x, MAP_OY + 56);
}

animate()
{
    int frame, sx1, sx2;

    for (frame = 0; frame < 144; frame++) {
        sx1 = MAP_OX + (frame * 2) % (MAP_COLS * TILE_W);
        sx2 = MAP_OX + MAP_COLS * TILE_W - 8
              - (frame * 2) % (MAP_COLS * TILE_W);
        frame(sx1, sx2);
        nap(2);
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "tiles: window failed\n");
        exit(1);
    }

    palinit();
    frame(MAP_OX, MAP_OX + MAP_COLS * TILE_W - 8);
    animate();

    close(g_win);
    exit(0);
}
