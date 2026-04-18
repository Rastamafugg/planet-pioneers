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
    set_palette(COLOR_BLACK, 0x00);
    set_palette(COLOR_GREEN, 0x12);
    set_palette(COLOR_BLUE,  0x09);
    set_palette(COLOR_WHITE, 0x3f);
}

clear_screen()
{
    fill_rect(0, 0, SCR_W, SCR_H, COLOR_BLACK);
}

draw_plains_tile(x, y)
int x, y;
{
    fill_rect(x, y, TILE_W, TILE_H, COLOR_GREEN);
    fill_rect(x + 2, y + 7, 12, 2, COLOR_BLACK);
}

draw_river_tile(x, y)
int x, y;
{
    fill_rect(x, y, TILE_W, TILE_H, COLOR_BLUE);
    fill_rect(x + 6, y, 4, TILE_H, COLOR_WHITE);
    fill_rect(x + 7, y, 2, TILE_H, COLOR_BLUE);
}

draw_mountain_tile(x, y)
int x, y;
{
    fill_rect(x, y, TILE_W, TILE_H, COLOR_GREEN);
    fill_rect(x + 6, y + 2, 4, 3, COLOR_WHITE);
    fill_rect(x + 4, y + 5, 8, 3, COLOR_WHITE);
    fill_rect(x + 2, y + 8, 12, 4, COLOR_WHITE);
    fill_rect(x + 7, y + 2, 2, 3, COLOR_BLACK);
}

draw_tile(kind, x, y)
int kind, x, y;
{
    if (kind == 1) {
        draw_river_tile(x, y);
    } else if (kind == 2) {
        draw_mountain_tile(x, y);
    } else {
        draw_plains_tile(x, y);
    }
}

draw_map()
{
    int r, c;

    for (r = 0; r < MAP_ROWS; r++)
        for (c = 0; c < MAP_COLS; c++)
            draw_tile(g_map[r][c],
                      MAP_OX + c * TILE_W,
                      MAP_OY + r * TILE_H);
}

draw_player(x, y)
int x, y;
{
    fill_rect(x + 2, y,     4, 2, COLOR_WHITE);
    fill_rect(x + 1, y + 2, 6, 2, COLOR_WHITE);
    fill_rect(x + 3, y + 4, 2, 1, COLOR_WHITE);
    fill_rect(x,     y + 5, 8, 2, COLOR_WHITE);
    fill_rect(x + 1, y + 7, 2, 1, COLOR_WHITE);
    fill_rect(x + 5, y + 7, 2, 1, COLOR_WHITE);
}

draw_mule(x, y)
int x, y;
{
    fill_rect(x + 1, y + 1, 6, 3, COLOR_GREEN);
    fill_rect(x,     y + 4, 8, 2, COLOR_GREEN);
    fill_rect(x,     y + 6, 2, 2, COLOR_GREEN);
    fill_rect(x + 6, y + 6, 2, 2, COLOR_GREEN);
    fill_rect(x + 5, y,     3, 1, COLOR_WHITE);
}

render_frame(player_x, mule_x)
int player_x, mule_x;
{
    clear_screen();
    draw_map();
    draw_player(player_x, MAP_OY + 4);
    draw_mule(mule_x, MAP_OY + 56);
}

animate()
{
    int frame, sx1, sx2;

    for (frame = 0; frame < 144; frame++) {
        sx1 = MAP_OX + (frame * 2) % (MAP_COLS * TILE_W);
        sx2 = MAP_OX + MAP_COLS * TILE_W - 8
              - (frame * 2) % (MAP_COLS * TILE_W);
        render_frame(sx1, sx2);
        sleep_ticks(2);
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "tiles: window failed\n");
        exit(1);
    }

    init_palette();
    render_frame(MAP_OX, MAP_OX + MAP_COLS * TILE_W - 8);
    animate();

    close(g_win);
    exit(0);
}
