/***********************************************************************
 * poc_input.c  -  SS.KySns realtime keyboard sense PoC.
 *
 * Polls SS.KySns ($27) on stdin (path 0) for ~5 seconds, printing the
 * raw bit pattern and decoded edge events (press/release) for arrows,
 * space, shift, ctrl, alt. Confirms the call works under EOU before
 * input.c integrates with the main loop.
 *
 * Tech Ref §SS.KySns: A=path, B=$27 -> A returns
 *   bit0 SHIFT  bit1 CTRL  bit2 ALT  bit3 UP  bit4 DOWN
 *   bit5 LEFT   bit6 RIGHT bit7 SPACE
 *
 * Compile: dcc poc_input.c -m=4k -f=/dd/cmds/pocinput
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define SS_KYSNS  0x27
#define I_GETSTT  0x8D
#define F_SLEEP   0x0A

#define K_SHIFT  0x01
#define K_CTRL   0x02
#define K_ALT    0x04
#define K_UP     0x08
#define K_DOWN   0x10
#define K_LEFT   0x20
#define K_RIGHT  0x40
#define K_SPACE  0x80

int kysns(state)
unsigned char *state;
{
    struct registers r;

    r.rg_a = 0;
    r.rg_b = SS_KYSNS;
    if (_os9(I_GETSTT, &r)) return -1;
    *state = r.rg_a & 0xff;
    return 0;
}

sleep3()
{
    struct registers r;
    r.rg_x = 3;
    _os9(F_SLEEP, &r);
}

report(cur, prev)
unsigned char cur;
unsigned char prev;
{
    unsigned char down, up;

    down = cur & ~prev;
    up = ~cur & prev;
    if (!down && !up) return;

    printf("kys=%02x", cur & 0xff);
    if (down & K_UP)    printf(" +UP");
    if (down & K_DOWN)  printf(" +DN");
    if (down & K_LEFT)  printf(" +LF");
    if (down & K_RIGHT) printf(" +RT");
    if (down & K_SPACE) printf(" +SP");
    if (down & K_SHIFT) printf(" +SH");
    if (down & K_CTRL)  printf(" +CT");
    if (down & K_ALT)   printf(" +AL");
    if (up & K_UP)      printf(" -UP");
    if (up & K_DOWN)    printf(" -DN");
    if (up & K_LEFT)    printf(" -LF");
    if (up & K_RIGHT)   printf(" -RT");
    if (up & K_SPACE)   printf(" -SP");
    if (up & K_SHIFT)   printf(" -SH");
    if (up & K_CTRL)    printf(" -CT");
    if (up & K_ALT)     printf(" -AL");
    printf("\n");
}

main()
{
    int i;
    unsigned char cur, prev;

    prev = 0;
    cur = 0;
    printf("poc_input: press arrows/space/shift/ctrl/alt for 5s\n");
    /* 100 polls * 3 ticks = 300 ticks = 5 sec */
    for (i = 0; i < 100; i++) {
        if (kysns(&cur)) {
            printf("poc_input: SS.KySns failed\n");
            exit(1);
        }
        report(cur, prev);
        prev = cur;
        sleep3();
    }
    printf("poc_input: done\n");
    exit(0);
}
