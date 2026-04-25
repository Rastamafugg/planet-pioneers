/***********************************************************************
 * poc_shmemc.c  -  Shared-memory PoC child (phase 2b).
 *
 * Reads block# and parent PID from argv. Maps the block into its
 * own DAT image, verifies magic, increments the counter, signals
 * SIG_SHMEM_ACK (131) back to the parent, unmaps, exits.
 *
 * Compile: dcc poc_shmemc.c -m=4k -f=/dd/cmds/pocshmc
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef F_SEND
#define F_SEND   0x08
#endif
#ifndef F_MAPBLK
#define F_MAPBLK 0x3D
#endif
#ifndef F_CLRBLK
#define F_CLRBLK 0x38
#endif

#define SIG_SHMEM_ACK 131
#define MAGIC         0x5AA5

main(argc, argv)
int argc;
char *argv[];
{
    struct registers r;
    int   block, parent_pid;
    unsigned int *shared;

    if (argc < 3) {
        printf("pocshmc: need block# and pid\n");
        exit(1);
    }
    block      = atoi(argv[1]);
    parent_pid = atoi(argv[2]);

    /* map the same physical block into our address space */
    r.rg_b = 1;
    r.rg_x = block;
    if (_os9(F_MAPBLK, &r)) {
        printf("pocshmc: F$MapBlk b=%d err #%d\n",
               block, r.rg_b & 0xff);
        exit(2);
    }
    shared = (unsigned int *)r.rg_u;

    if ((int)shared[0] != MAGIC) {
        printf("pocshmc: bad magic %x (want %x)\n",
               (int)shared[0], MAGIC);
        /* fall through to unmap + signal so parent does not hang */
    } else {
        shared[1] = shared[1] + 1;
    }

    /* signal parent */
    r.rg_a = parent_pid;
    r.rg_b = SIG_SHMEM_ACK;
    if (_os9(F_SEND, &r)) {
        printf("pocshmc: F$Send pid=%d err #%d\n",
               parent_pid, r.rg_b & 0xff);
    }

    /* unmap from our own address space (parent will F$DelRAM) */
    r.rg_b = 1;
    r.rg_u = (unsigned)shared;
    _os9(F_CLRBLK, &r);

    exit(0);
}
