/***********************************************************************
 * poc_rnd.c  -  Render API smoke driver (phase 4 PoC parent).
 *
 * Brings up the render child, draws the embedded map onto BOTH screens
 * once (one ren_dmap + ren_pres pair per screen — the page flip swaps
 * which screen is back), then drives sprite movement frame-by-frame.
 * Each frame enqueues only sprite moves + present, mirroring
 * poc_cvdg16.c's render_step. The child does save_bg/rest_bg per
 * (screen, slot) so only sprite footprints are touched per frame.
 *
 * Compile: dcc poc_rnd.c render.c -m=4k -f=/dd/cmds/pocrnd
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef F_TIME
#define F_TIME 0x15
#endif

#define MAP_OX     2
#define MAP_OY     1
#define TILE_W     17
#define TILE_H     38

#define NFRAMES    300
#define PATHFRAMES 150

#define MOVE_X0    MAP_OX
#define MOVE_Y0    (MAP_OY + 8)
#define MOVE_X1    146
#define MOVE_Y1    (MAP_OY + TILE_H * 3)

extern int ren_init();
extern int ren_dmap();
extern int ren_spr();
extern int ren_pres();
extern int ren_flush();
extern int ren_shut();

int nowsec()
{
    struct registers r;
    unsigned char pkt[6];
    r.rg_x = (unsigned)pkt;
    if (_os9(F_TIME, &r)) return -1;
    return ((int)pkt[4]) * 60 + (int)pkt[5];
}

int evenx(x)
int x;
{
    return (x / 2) * 2;
}

int interp(a, b, t, span)
int a, b, t, span;
{
    if (span <= 0) return b;
    return a + ((b - a) * t) / span;
}

pathpos(frame, mule, xp, yp, dirp)
int frame, mule, *xp, *yp, *dirp;
{
    int seg, st, en, span, t;

    seg = (frame * 4) / PATHFRAMES;
    if (seg > 3) seg = 3;
    st = (seg * PATHFRAMES) / 4;
    en = ((seg + 1) * PATHFRAMES) / 4 - 1;
    span = en - st;
    t = frame - st;

    if (!mule) {
        if (seg == 0)      { *xp = evenx(interp(MOVE_X0, MOVE_X1, t, span)); *yp = MOVE_Y0; *dirp = 0; }
        else if (seg == 1) { *xp = MOVE_X1; *yp = interp(MOVE_Y0, MOVE_Y1, t, span); *dirp = 1; }
        else if (seg == 2) { *xp = evenx(interp(MOVE_X1, MOVE_X0, t, span)); *yp = MOVE_Y1; *dirp = 2; }
        else               { *xp = MOVE_X0; *yp = interp(MOVE_Y1, MOVE_Y0, t, span); *dirp = 3; }
    } else {
        if (seg == 0)      { *xp = evenx(interp(MOVE_X0, MOVE_X1, t, span)); *yp = MOVE_Y1; *dirp = 0; }
        else if (seg == 1) { *xp = MOVE_X1; *yp = interp(MOVE_Y1, MOVE_Y0, t, span); *dirp = 3; }
        else if (seg == 2) { *xp = evenx(interp(MOVE_X1, MOVE_X0, t, span)); *yp = MOVE_Y0; *dirp = 2; }
        else               { *xp = MOVE_X0; *yp = interp(MOVE_Y0, MOVE_Y1, t, span); *dirp = 1; }
    }
}

main()
{
    int err, frame, start, end, elapsed;
    int px, py, pd, mx, my, md;

    err = ren_init();
    if (err) { printf("poc_rnd: ren_init err %d\n", err); exit(1); }
    printf("poc_rnd: render up; %d frames\n", NFRAMES);

    /* Seed both screens with the map. The child's R_OP_DRAWMAP draws
     * to the (offscreen) back buffer and then clones it to the front
     * via 16K memcpy — much faster than re-running the procedural
     * draw twice. ren_pres then flips, surfacing the seeded buffer. */
    ren_dmap();
    ren_pres();
    ren_flush();

    start = nowsec();
    for (frame = 0; frame < NFRAMES; frame++) {
        pathpos(frame / 2, 0, &px, &py, &pd);
        pathpos(frame / 2, 1, &mx, &my, &md);

        ren_spr(0, px, py, frame / 2,     pd, 0);
        ren_spr(1, mx, my, frame / 2 + 1, md, 1);
        ren_pres();
        ren_flush();
    }
    end = nowsec();

    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        if (elapsed > 0)
            printf("poc_rnd: frames=%d sec=%d fps=%d\n",
                   frame, elapsed, frame / elapsed);
        else
            printf("poc_rnd: frames=%d sec=<1\n", frame);
    }

    ren_shut();
    printf("poc_rnd: clean\n");
    exit(0);
}
