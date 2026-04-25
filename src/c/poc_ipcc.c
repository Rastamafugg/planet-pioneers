/***********************************************************************
 * poc_ipcc.c  -  IPC PoC child (phase 2a).
 *
 * Reads parent PID from argv[1], sends SIG_IPC_ACK (130) via F$Send,
 * exits 0.
 *
 * Compile: dcc poc_ipcc.c -m=4k -f=/dd/cmds/pocipcc
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef F_SEND
#define F_SEND      0x08
#endif
#define SIG_IPC_ACK 130

main(argc, argv)
int argc;
char *argv[];
{
    struct registers r;
    int parent_pid;

    if (argc < 2) {
        printf("pocipcc: missing parent pid\n");
        exit(1);
    }
    parent_pid = atoi(argv[1]);

    r.rg_a = parent_pid;
    r.rg_b = SIG_IPC_ACK;
    if (_os9(F_SEND, &r)) {
        printf("pocipcc: F$Send pid=%d err #%d\n",
               parent_pid, r.rg_b & 0xff);
        exit(2);
    }
    exit(0);
}
