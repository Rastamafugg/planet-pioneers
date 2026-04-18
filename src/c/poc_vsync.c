/***********************************************************************
 * poc_vsync.c  -  NitrOS-9 /nil VRN 1/60 second VIRQ timing PoC
 *
 * Uses documented VRN SS.FSet on /nil to receive a user signal every
 * 1/60 second, then waits for 120 ticks and reports measured timing.
 *
 * Compile: dcc poc_vsync.c -s -m=4k -f=/dd/cmds/pocvsync
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define VS_SIG    0x7f
#define VS_TICKS  120

#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif
#ifndef F_TIME
#define F_TIME    0x15
#endif
#ifndef I_SETSTT
#define I_SETSTT  0x8E
#endif
#ifndef SS_FSET
#define SS_FSET   0xC7
#endif

int open(), close(), intercept();

static int vpath;
static int vcnt;

vtrap(sig)
int sig;
{
    if (sig == VS_SIG) {
        vcnt++;
        return;
    }
    exit(sig);
}

int nowsec()
{
    struct registers r;
    unsigned char pkt[6];

    r.rg_x = (unsigned)pkt;
    if (_os9(F_TIME, &r)) return -1;
    return ((int)pkt[4]) * 60 + (int)pkt[5];
}

int vinit()
{
    struct registers r;

    vpath = open("/nil", 3);
    if (vpath < 0) {
        printf("poc_vsync: /nil open failed\n");
        return -1;
    }

    intercept(vtrap);
    r.rg_a = (char)vpath;
    r.rg_b = (char)SS_FSET;
    r.rg_x = 1;
    r.rg_y = 1;
    r.rg_u = VS_SIG;
    if (_os9(I_SETSTT, &r)) {
        printf("poc_vsync: SS.FSet error #%d\n", r.rg_b & 0xff);
        close(vpath);
        vpath = -1;
        return -1;
    }
    return 0;
}

vwait()
{
    int next;
    struct registers r;

    next = vcnt + 1;
    while (vcnt < next) {
        r.rg_x = 0;
        _os9(F_SLEEP, &r);
    }
}

main()
{
    int i, start, end, elapsed;

    vpath = -1;
    vcnt = 0;
    if (vinit()) exit(1);

    start = nowsec();
    for (i = 0; i < VS_TICKS; i++) vwait();
    end = nowsec();

    if (vpath >= 0) close(vpath);

    if (start >= 0 && end >= 0) {
        elapsed = end - start;
        if (elapsed < 0) elapsed += 3600;
        printf("poc_vsync: ticks=%d seconds=%d\n", vcnt, elapsed);
    } else {
        printf("poc_vsync: ticks=%d F$Time unavailable\n", vcnt);
    }
    exit(0);
}
