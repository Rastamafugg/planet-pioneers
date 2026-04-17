/***********************************************************************
 * poc_gfx.c  -  NitrOS-9 EOU / CoCo 3  Graphics PoC
 *
 * Verifies:
 *   - DWSet window configuration (type 6: 320x200 4-color, 2bpp)
 *   - SS.ScInf + F$MapBlk: screen mapped into process address space
 *   - 16 KB back buffer (malloc) as simulated double buffer
 *   - memcpy-based flip (front <- back)
 *   - Animated rectangle: visual confirmation of no tearing at 60 Hz
 *
 * Pixel format: 2bpp, 4 px/byte, MSB = leftmost pixel
 *   byte: [px0:7:6][px1:5:4][px2:3:2][px3:1:0]
 *
 * Compile:  dcc poc_gfx.c -m=24k -o=poc_gfx
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

/* Screen geometry */
#define SCR_W     320
#define SCR_H     200
#define SCR_BPR   80        /* bytes/row = 320px / 4px-per-byte      */
#define SCR_SIZE  16000     /* = SCR_BPR * SCR_H                     */
#define SCR_TYPE  6
#define SCR_COLS  40        /* character columns for DWSet            */
#define SCR_ROWS  25        /* character rows    for DWSet            */

/* OS-9 system call codes */
#define F_SLEEP   0x0A
#define F_MAPBLK  0x4F
#define F_CLRBLK  0x50
#define I_GETSTT  0x8D
#define SS_SCINF  0x8F      /* SS.ScInf GetStat code                 */

/* Forward declarations (implicit int in K&R; listed for clarity) */
int open(), write(), close();

static int            g_win;
static unsigned char *g_scr;    /* mapped front buffer               */
static unsigned char *g_back;   /* malloc'd back buffer              */
static unsigned char  g_nblks;  /* MMU block count from SS.ScInf     */

/* ------------------------------------------------------------------
 * Pixel operations
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

/* Fast fill: requires x and w both 4-pixel-aligned; falls back otherwise */
void fill_hline(buf, x, y, w, c)
unsigned char *buf; int x, y, w, c;
{
    unsigned char pk;
    pk = c & 3; pk |= (pk << 2); pk |= (pk << 4); /* pack color into all 4 slots */

    if (((x | w) & 3) == 0) {
        unsigned char *p = buf + y * SCR_BPR + (x >> 2);
        int n = w >> 2;
        while (n--) *p++ = pk;
    } else {
        int i;
        for (i = 0; i < w; i++) put_pixel(buf, x + i, y, c);
    }
}

void fill_rect(buf, x, y, w, h, c)
unsigned char *buf; int x, y, w, h, c;
{
    int r;
    for (r = 0; r < h; r++) fill_hline(buf, x, y + r, w, c);
}

void flip()
{
    memcpy(g_scr, g_back, SCR_SIZE);
}

/* ------------------------------------------------------------------
 * Window management
 * ------------------------------------------------------------------ */

int open_window()
{
    unsigned char cmd[10];
    struct registers r;

    g_win = open("/w1", 3);
    if (g_win < 0) return -1;

    /* DWSet: ESC 0x20 STY CPX CPY SZX SZY PRN_FG PRN_BG PRN_BORDER */
    cmd[0]=0x1B; cmd[1]=0x20; cmd[2]=SCR_TYPE;
    cmd[3]=0;    cmd[4]=0;    cmd[5]=SCR_COLS; cmd[6]=SCR_ROWS;
    cmd[7]=0;    cmd[8]=1;    cmd[9]=0;
    write(g_win, cmd, 10);

    /* Wait 2 ticks for CoWin to process DWSet and allocate screen RAM */
    r.rg_x = 2; _os9(F_SLEEP, &r);
    return 0;
}

void set_palette(prn, ctn)  int prn, ctn;
{
    /* ESC 0x31 PRN CTN */
    /* CTN: 6-bit RGB, bits [5:4]=Rhi,Rlo [3:2]=Ghi,Glo [1:0]=Bhi,Blo */
    unsigned char cmd[4];
    cmd[0]=0x1B; cmd[1]=0x31;
    cmd[2]=(unsigned char)prn;
    cmd[3]=(unsigned char)ctn;
    write(g_win, cmd, 4);
}

void init_palette()
{
    set_palette(0, 0x00);   /* 0 = black                             */
    set_palette(1, 0x12);   /* 1 = green  (plains terrain)           */
    set_palette(2, 0x09);   /* 2 = blue   (river terrain)            */
    set_palette(3, 0x3F);   /* 3 = white  (highlights / UI)          */
}

int map_screen()
{
    struct registers r;
    unsigned char nb, sb;

    /* SS.ScInf: A = num_blocks, B = start_block_number */
    r.rg_a = (char)g_win;
    r.rg_b = (char)SS_SCINF;
    if (_os9(I_GETSTT, &r)) return -1;

    nb = (unsigned char)r.rg_a;
    sb = (unsigned char)r.rg_b;
    g_nblks = nb;

    /* F$MapBlk: B = num_blocks, X = start_block => U = process addr */
    r.rg_b = (char)nb;
    r.rg_x = (unsigned)sb;
    if (_os9(F_MAPBLK, &r)) return -1;

    g_scr = (unsigned char *)r.rg_u;
    return 0;
}

void unmap_screen()
{
    /* F$ClrBlk: B = num_blocks, U = mapped address */
    struct registers r;
    r.rg_b = (char)g_nblks;
    r.rg_u = (unsigned)g_scr;
    _os9(F_CLRBLK, &r);
}

/* ------------------------------------------------------------------
 * Animation
 * ------------------------------------------------------------------ */

void draw_background()
{
    /* Four horizontal color bands */
    fill_rect(g_back,  0,   0, SCR_W,  50, 1);
    fill_rect(g_back,  0,  50, SCR_W,  50, 2);
    fill_rect(g_back,  0, 100, SCR_W,  50, 1);
    fill_rect(g_back,  0, 150, SCR_W,  50, 2);
}

void animate()
{
    struct registers r;
    int frame, bx;

    for (frame = 0; frame < 180; frame++) {
        bx = (frame * 2) % (SCR_W - 32);

        draw_background();
        fill_rect(g_back, bx, 80, 32, 40, 3);  /* white bar (front)  */
        fill_rect(g_back, bx + 4, 88, 24, 24, 0); /* black hole inside */

        flip();

        r.rg_x = 1; _os9(F_SLEEP, &r);         /* yield 1 tick ~1/60 s */
    }
}

main()
{
    if (open_window()) {
        fprintf(stderr, "poc_gfx: /w1 open failed\n");
        exit(1);
    }
    init_palette();

    g_back = (unsigned char *)malloc(SCR_SIZE);
    if (!g_back) {
        fprintf(stderr, "poc_gfx: malloc failed\n");
        close(g_win); exit(1);
    }
    memset(g_back, 0, SCR_SIZE);

    if (map_screen()) {
        fprintf(stderr, "poc_gfx: SS.ScInf or F$MapBlk failed\n");
        free(g_back); close(g_win); exit(1);
    }

    flip();       /* clear screen before first frame */
    animate();

    unmap_screen();
    free(g_back);
    close(g_win);
    exit(0);
}

/*
 * ====================================================================
 * GIME PAGE-FLIP (type 8 / 16-color / 32 KB) — no back buffer needed
 * ====================================================================
 * Setup:
 *   1. SS.ScInf on the type-8 window path.
 *      A = 4 (blocks), B = page_a_start_block.
 *   2. F$AlHRAM with B=4 => D = page_b_start_block (high RAM).
 *   3. F$MapBlk(page_a, 4) => addr_a  (render target A)
 *   4. F$MapBlk(page_b, 4) => addr_b  (render target B)
 *   5. Alternate: render to addr_b while addr_a is displayed,
 *      then flip; render to addr_a while addr_b is displayed.
 *
 * Flip (write GIME video-start register $FF9D):
 *   NitrOS-9 Level 2 user processes can access $FF00-$FFFF (I/O space).
 *   Physical address of page = block_number * 8192.
 *   GIME $FF9D[6:0] = (physical_address >> 9) & 0x7F
 *
 *   unsigned char *ff9d = (unsigned char *)0xFF9D;
 *   page_b_ff9d = (page_b_start_block * 8192) >> 9;  (= page_b_start_block * 16)
 *   *ff9d = (*ff9d & 0x80) | (page_b_ff9d & 0x7F);
 *
 *   Zero memcpy. Zero tearing. Under 10 CPU cycles per flip.
 *   Verify: read $FF9D before first write to preserve bit 7 (GIME config).
 * ====================================================================
 */