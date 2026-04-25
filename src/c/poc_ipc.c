/***********************************************************************
 * poc_ipc.c  -  IPC PoC parent (phase 2a).
 *
 * Forks pocipcc, passing parent PID in the param area. Installs a
 * signal intercept via slpicpt and waits up to ~3s for SIG_IPC_ACK
 * (130) from the child. Reaps the child via F$Wait.
 *
 * Resolves wiki/platform/ipc.md open question #3 by exercising direct
 * _os9(F$Fork) (vs. libc system()/os9fork()).
 *
 * Compile: dcc poc_ipc.c slpicpt.c -m=4k -f=/dd/cmds/pocipc
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

#ifndef F_ID
#define F_ID    0x0C
#endif
#ifndef F_FORK
#define F_FORK  0x03
#endif
#ifndef F_WAIT
#define F_WAIT  0x04
#endif
#ifndef F_SEND
#define F_SEND  0x08
#endif
#ifndef F_SLEEP
#define F_SLEEP 0x0A
#endif

#define SIG_IPC_ACK 130
#define POLL_TICKS  180   /* ~3s @ 60Hz */

extern int slpicpt();

main()
{
    struct registers r;
    char  cmem[1];
    char  name[8];
    char  param[16];
    int   action, sigout, cntout, okout;
    int   my_pid;
    int   tries;
    int   plen;

    /* own PID */
    if (_os9(F_ID, &r)) {
        printf("poc_ipc: F$ID failed\n");
        exit(1);
    }
    my_pid = r.rg_a & 0xff;
    printf("poc_ipc: parent pid=%d\n", my_pid);

    /* install intercept */
    action = 1;
    slpicpt(0, cmem, 1, &action, 2, &sigout, 2, &cntout, 2, &okout, 2);
    if (!okout) {
        printf("poc_ipc: slpicpt install failed\n");
        exit(2);
    }

    /* build child param: "<pid>\r" */
    sprintf(param, "%d", my_pid);
    plen = strlen(param);
    param[plen++] = '\r';

    /* child name "pocipcc" with high bit on last char */
    name[0] = 'p'; name[1] = 'o'; name[2] = 'c';
    name[3] = 'i'; name[4] = 'p'; name[5] = 'c';
    name[6] = 'c' | 0x80;

    /* F$Fork: A=0(lang), B=namelen, X=name, Y=paramlen, U=param */
    r.rg_a = 0;
    r.rg_b = 7;
    r.rg_x = (unsigned)name;
    r.rg_y = plen;
    r.rg_u = (unsigned)param;
    if (_os9(F_FORK, &r)) {
        printf("poc_ipc: F$Fork err #%d\n", r.rg_b & 0xff);
        exit(3);
    }
    printf("poc_ipc: forked child pid=%d\n", r.rg_a & 0xff);

    /* poll for ack */
    sigout = 0;
    for (tries = 0; tries < POLL_TICKS; tries++) {
        action = 2;
        slpicpt(0, cmem, 1, &action, 2, &sigout, 2, &cntout, 2, &okout, 2);
        if (sigout == SIG_IPC_ACK) break;
        r.rg_x = 1;
        _os9(F_SLEEP, &r);
    }

    if (sigout == SIG_IPC_ACK) {
        printf("poc_ipc: ack sig=%d hits=%d (after %d ticks)\n",
               sigout, cntout, tries);
    } else {
        printf("poc_ipc: timeout last_sig=%d hits=%d\n", sigout, cntout);
    }

    /* reap child */
    if (_os9(F_WAIT, &r)) {
        printf("poc_ipc: F$Wait err #%d\n", r.rg_b & 0xff);
    } else {
        printf("poc_ipc: child exit=%d\n", r.rg_b & 0xff);
    }
    exit(0);
}
