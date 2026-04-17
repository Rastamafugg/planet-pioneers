/***********************************************************************
 * poc_tiles.c  -  NitrOS-9 EOU / CoCo 3  Tile & Sprite PoC
 *
 * Verifies:
 *   - Static tile data (3 types x 64 bytes: plains, river, mountain)
 *   - Static sprite data (2 sprites x 16 bytes; color 0 = transparent)
 *   - tile_blit(): 4-aligned fast-path (direct byte copy, no per-pixel loop)
 *   - sprite_blit(): per-pixel transparency check
 *   - 9x5 tile grid centered on screen (matches M.U.L.E. map structure)
 *   - Two animated sprites overlaid on static tile grid
 *
 * Tile format  (2bpp): 16x16 px = 4 bytes/row x 16 rows = 64 bytes
 * Sprite format(2bpp):  8x8 px = 2 bytes/row x  8 rows = 16 bytes
 *   color 0 = transparent (not written to buffer)
 *
 * NOTE: static initialized arrays go into the process data segment
 * (initialized data section, copied from module at startup).
 * Production: use a linked NitrOS-9 data module for tile/sprite ROMs
 * to share across processes and eliminate per-process data cost.
 *
 * Compile:  dcc poc_tiles.c -m=24k -o=poc_tiles
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

/* Screen geometry (type 6: 320x200 4-color) */
#define SCR_W     320
#define SCR_H     200
#define SCR_BPR   80
#define SCR_SIZE  16000
#define SCR_TYPE  6
#define SCR_COLS  40
#define SCR_ROWS  25

/* Tile geometry */
#define TILE_W    16
#define TILE_H    16
#define TILE_BPR  4           /* 16px / 4px-per-byte                 */
#define TILE_SZ   64          /* TILE_BPR * TILE_H                   */

/* Sprite geometry */
#define SPR_W     8
#define SPR_H     8
#define SPR_BPR   2           /* 8px / 4px-per-byte                  */
#define SPR_SZ    16          /* SPR_BPR * SPR_H                     */

/* Map layout in tiles (9 columns x 5 rows) */
#define MAP_COLS  9
#define MAP_ROWS  5

/* Pixel origin: center map grid on screen */
#define MAP_OX    ((SCR_W - MAP_COLS * TILE_W) / 2)    /* = 88 px   */
#define MAP_OY    ((SCR_H - MAP_ROWS * TILE_H) / 2)    /* = 60 px   */

/* OS-9 codes */
#define F_SLEEP   0x0A
#define F_MAPBLK  0x4F
#define F_CLRBLK  0x50
#define I_GETSTT  0x8D
#define SS_SCINF  0x8F

int open(), write(), close();

static int            g_win;
static unsigned char *g_scr;
static unsigned char *g_back;
static unsigned char  g_nblks;

/* ------------------------------------------------------------------
 * Tile data (initialized static: in process data segment)
 * Palette: 0=black, 1=green, 2=blue, 3=white
 *
 * 2bpp row encoding:
 *   byte = [px0:7:6][px1:5:4][px2:3:2][px3:1:0]
 *   all-green row : 01010101 = 0x55
 *   all-blue  row : 10101010 = 0xAA
 *   all-white row : 11111111 = 0xFF
 *   all-black row : 00000000 = 0x00
 * ------------------------------------------------------------------ */

/* Tile 0: Plains */
static unsigned char tile_plains[TILE_SZ] = {
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55,
    0x55,0x55,0x55,0x55,  0x55,0x55,0x55,0x55
};

/* Tile 1: River */
static unsigned char tile_river[TILE_SZ] = {
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA,
    0xAA,0xAA,0xAA,0xAA,  0xAA,0xAA,0xAA,0xAA
};

/*
 * Tile 2: Mountain
 * Row 0 peak example (16px = 4 bytes):
 *   px 0-3  = green: 0x55
 *   px 4-7  = green,green,white,white: 01 01 11 11 = 0x5F
 *   px 8-11 = white,white,green,green: 11 11 01 01 = 0xF5
 *   px 12-15= green: 0x55
 */
static unsigned char tile_mountain[TILE_SZ] = {
    /* row  0 */ 0x55,0x5F,0xF5,0x55,
    /* row  1 */ 0x55,0xFF,0xFF,0x55,
    /* row  2 */ 0x5F,0xFF,0xFF,0xF5,
    /* row  3 */ 0xFF,0xFF,0xFF,0xFF,
    /* row  4 */ 0xFF,0xFF,0xFF,0xFF,
    /* row  5 */ 0x5F,0xFF,0xFF,0xF5,
    /* row  6 */ 0x55,0x55,0x55,0x55,
    /* row  7 */ 0x55,0x55,0x55,0x55,
    /* row  8 */ 0x55,0x55,0x55,0x55,
    /* row  9 */ 0x55,0x55,0x55,0x55,
    /* row 10 */ 0x55,0x55,0x55,0x55,
    /* row 11 */ 0x55,0x55,0x55,0x55,
    /* row 12 */ 0x55,0x55,0x55,0x55,
    /* row 13 */ 0x55,0x55,0x55,0x55,
    /* row 14 */ 0x55,0x55,0x55,0x55,
    /* row 15 */ 0x55,0x55,0x55,0x55
};

static unsigned char *tile_table[3];   /* populated in main() */

/* ------------------------------------------------------------------
 * Sprite data
 * 2bpp, 8px/row = 2 bytes/row; color 0 = transparent
 *
 * Sprite 0: humanoid player (color 3 = white body)
 *   row 0: [0][0][3][3][3][3][0][0]
 *          00 00 11 11 | 11 11 00 00 = 0x0F 0xF0
 * ------------------------------------------------------------------ */
static unsigned char spr_player[SPR_SZ] = {
    0x0F,0xF0,  /* row 0: ..WW WW..  head top   */
    0x3F,0xFC,  /* row 1: .WWWWWW.  head        */
    0x0F,0xF0,  /* row 2: ..WW WW.  neck        */
    0xFF,0xFF,  /* row 3: WWWWWWWW  torso       */
    0xFF,0xFF,  /* row 4: WWWWWWWW  torso       */
    0x3C,0x3C,  /* row 5: .WW..WW.  waist       */
    0xC3,0xC3,  /* row 6: WW....WW  legs        */
    0xC3,0xC3   /* row 7: WW....WW  feet        */
};

/* Sprite 1: MULE (color 1 = green body) */
static unsigned char spr_mule[SPR_SZ] = {
    0x00,0x00,  /* row 0: ........                */
    0x5F,0xF5,  /* row 1: .GGGGGG.  back/body top */
    0x5F,0xF5,  /* row 2: .GGGGGG.               */
    0x5F,0xF5,  /* row 3: .GGGGGG.               */
    0x55,0x55,  /* row 4: GGGGGGGG  belly         */
    0x55,0x55,  /* row 5: GGGGGGGG               */
    0x50,0x05,  /* row 6: GG....GG  legs          */
    0x50,0x05   /* row 7: GG....GG  feet          */
};

/* ------------------------------------------------------------------
 * Map layout: 0=plains, 1=river (col 4), 2=mountain
 * ------------------------------------------------------------------ */
static unsigned char g_map[MAP_ROWS][MAP_COLS] = {
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 0, 1, 0, 0, 0, 2 },
    { 0, 0, 2, 0, 1, 0, 2, 0, 0 },
    { 2, 0, 0, 2, 1, 2, 0, 0, 2 },
    { 0, 2, 0, 0, 1, 0, 0, 2, 0 }
};

/* ------------------------------------------------------------------
 * Pixel / blit operations
 * ------------------------------------------------------------------ */

void put_pixel(buf, x, y, c)
unsigned char *buf; int x, y, c;
{
    unsigned char *p; int sh;
    if ((unsigned)x >= SCR_W || (unsigned)y >= SCR_H) return;
    p  = buf + y * SCR_BPR + (x >> 2);
    sh = 6 - ((x & 3) << 1);
    *p = (*p & ~(3 << sh)) | ((c & 3) << sh);
}

/*
 * Blit 16x16 tile to pixel (px, py).
 * Fast path active when px is 4-pixel-aligned (always true for
 * tile-column positions MAP_OX + col*16, since MAP_OX=88 and 88%4==0).
 */
void tile_blit(td, px, py)
unsigned char *td; int px, py;
{
    int row;
    unsigned char *dst;
    unsigned char *src;

    if ((unsigned)px + TILE_W > SCR_W) return;
    if ((unsigned)py + TILE_H > SCR_H) return;

    if ((px & 3) == 0) {
        /* 4-pixel-aligned: copy 4 bytes per row directly             */
        for (row = 0; row < TILE_H; row++) {
            src = td      + row * TILE_BPR;
            dst = g_back  + (py + row) * SCR_BPR + (px >> 2);
            dst[0]=src[0]; dst[1]=src[1];
            dst[2]=src[2]; dst[3]=src[3];
        }
    } else {
        /* Unaligned slow path (should not occur in normal tile grid)  */
        int col, sh;
        unsigned char *sp;
        for (row = 0; row < TILE_H; row++)
            for (col = 0; col < TILE_W; col++) {
                sp = td + row * TILE_BPR + (col >> 2);
                sh = 6 - ((col & 3) << 1);
                put_pixel(g_back, px+col, py+row, (*sp >> sh) & 3);
            }
    }
}

/*
 * Blit 8x8 sprite to pixel (px, py).
 * Color 0 = transparent: those pixels are not written to the buffer.
 * Negative px/py: bounds check in put_pixel handles via unsigned wrap.
 */
void sprite_blit(sd, px, py)
unsigned char *sd; int px, py;
{
    int row, col, sh, c;
    unsigned char *src;

    for (row = 0; row < SPR_H; row++) {
        src = sd + row * SPR_BPR;
        for (col = 0; col < SPR_W; col++) {
            sh = 6 - ((col & 3) << 1);
            c  = (src[col >> 2] >> sh) & 3;
            if (c) put_pixel(g_back, px + col, py + row, c);
        }
    }
}

/* ------------------------------------------------------------------
 * Rendering
 * ------------------------------------------------------------------ */

void render_map()
{
    int r, c;
    for (r = 0; r < MAP_ROWS; r++)
        for (c = 0; c < MAP_COLS; c++)
            tile_blit(tile_table[g_map[r][c]],
                      MAP_OX + c * TILE_W,
                      MAP_OY + r * TILE_H);
}

void flip()  { memcpy(g_scr, g_back, SCR_SIZE); }

/* ---- window management (same as poc_gfx.c) ---- */

int open_window()
{
    unsigned char cmd[10];
    struct registers r;
    g_win = open("/w1", 3);
    if (g_win < 0) return -1;
    cmd[0]=0x1B; cmd[1]=0x20; cmd[2]=SCR_TYPE;
    cmd[3]=0;    cmd[4]=0;    cmd[5]=SCR_COLS; cmd[6]=SCR_ROWS;
    cmd[7]=0;    cmd[8]=1;    cmd[9]=0;
    write(g_win, cmd, 10);
    r.rg_x = 2; _os9(F_SLEEP, &r);
    return 0;
}

void set_palette(prn, ctn)  int prn, ctn;
{
    unsigned char cmd[4];
    cmd[0]=0x1B; cmd[1]=0x31;
    cmd[2]=(unsigned char)prn; cmd[3]=(unsigned char)ctn;
    write(g_win, cmd, 4);
}

void init_palette()
{
    set_palette(0, 0x00); set_palette(1, 0x12);
    set_palette(2, 0x09); set_palette(3, 0x3F);
}

int map_screen()
{
    struct registers r;
    unsigned char nb, sb;
    r.rg_a=(char)g_win; r.rg_b=(char)SS_SCINF;
    if (_os9(I_GETSTT, &r)) return -1;
    nb=(unsigned char)r.rg_a; sb=(unsigned char)r.rg_b;
    g_nblks=nb;
    r.rg_b=(char)nb; r.rg_x=(unsigned)sb;
    if (_os9(F_MAPBLK, &r)) return -1;
    g_scr=(unsigned char *)r.rg_u;
    return 0;
}

void unmap_screen()
{
    struct registers r;
    r.rg_b=(char)g_nblks; r.rg_u=(unsigned)g_scr;
    _os9(F_CLRBLK, &r);
}

main()
{
    struct registers r;
    int frame, sx;

    /* Initialize tile lookup (cannot use initializer with array literals) */
    tile_table[0] = tile_plains;
    tile_table[1] = tile_river;
    tile_table[2] = tile_mountain;

    if (open_window()) { fprintf(stderr,"tiles: window failed\n"); exit(1); }
    init_palette();

    g_back = (unsigned char *)malloc(SCR_SIZE);
    if (!g_back) { fprintf(stderr,"tiles: malloc failed\n"); close(g_win); exit(1); }

    if (map_screen()) {
        fprintf(stderr,"tiles: screen map failed\n");
        free(g_back); close(g_win); exit(1);
    }

    /* Animate two sprites traversing the tile grid */
    for (frame = 0; frame < 144; frame++) {
        memset(g_back, 0, SCR_SIZE);
        render_map();

        /* Player sprite: left-to-right across map area */
        sx = MAP_OX + (frame * 2) % (MAP_COLS * TILE_W);
        sprite_blit(spr_player, sx, MAP_OY + 4);

        /* MULE sprite: right-to-left, lower row */
        sx = MAP_OX + MAP_COLS * TILE_W - 8
             - (frame * 2) % (MAP_COLS * TILE_W);
        sprite_blit(spr_mule, sx, MAP_OY + 56);

        flip();
        r.rg_x = 1; _os9(F_SLEEP, &r);
    }

    unmap_screen();
    free(g_back);
    close(g_win);
    exit(0);
}