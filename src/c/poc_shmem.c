/***********************************************************************
 * poc_shmem.c  -  Cross-process shared-memory PoC parent (phase 2b).
 *
 * Allocates one 8 KB physical block via F$AllRAM, maps it into our
 * own DAT image via F$MapBlk, writes a magic + counter header, then
 * forks `pocshmc` with the block number and our PID as parameters.
 * The child maps the same physical block into its own DAT image,
 * verifies the magic, increments the counter, and signals back; we
 * verify the counter advanced and reap.
 *
 * Resolves the C-side mechanics of the cross-process shared memory
 * pattern documented in wiki/platform/ipc.md. Gates phase 2b of the
 * roadmap.
 *
 * Compile: dcc poc_shmem.c slpicpt.c -m=4k -f=/dd/cmds/pocshm
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os9.h>

#ifndef F_ID
#define F_ID     0x0C
#endif
#ifndef F_FORK
#define F_FORK   0x03
#endif
#ifndef F_WAIT
#define F_WAIT   0x04
#endif
#ifndef F_SEND
#define F_SEND   0x08
#endif
#ifndef F_SLEEP
#define F_SLEEP  0x0A
#endif
#ifndef F_ALLRAM
#define F_ALLRAM 0x39
#endif
#ifndef F_MAPBLK
#define F_MAPBLK 0x3D
#endif
#ifndef F_CLRBLK
#define F_CLRBLK 0x38
#endif
#ifndef F_DELRAM
#define F_DELRAM 0x37
#endif

#define SIG_SHMEM_ACK 131
#define POLL_TICKS    180          /* ~3s */
#define MAGIC         0x5AA5

extern int slpicpt();

main()
{
    struct registers r;
    char  cmem[1];
    char  name[8];
    char  param[24];
    int   action, sigout, cntout, okout;
    int   my_pid;
    int   block;
    unsigned int *shared;
    int   tries;
    int   plen;

    /* own PID */
    if (_os9(F_ID, &r)) { printf("poc_shmem: F$ID failed\n"); exit(1); }
    my_pid = r.rg_a & 0xff;
    printf("poc_shmem: parent pid=%d\n", my_pid);

    /* allocate one 8K physical block */
    r.rg_b = 1;
    if (_os9(F_ALLRAM, &r)) {
        printf("poc_shmem: F$AllRAM err #%d\n", r.rg_b & 0xff);
        exit(2);
    }
    block = ((r.rg_a & 0xff) << 8) | (r.rg_b & 0xff);
    printf("poc_shmem: allocated block=%d\n", block);

    /* map into our address space */
    r.rg_b = 1;
    r.rg_x = block;
    if (_os9(F_MAPBLK, &r)) {
        printf("poc_shmem: F$MapBlk err #%d\n", r.rg_b & 0xff);
        exit(3);
    }
    shared = (unsigned int *)r.rg_u;
    printf("poc_shmem: mapped at %x\n", (int)shared);

    /* init shared header: magic + counter */
    shared[0] = MAGIC;
    shared[1] = 1;

    /* install intercept */
    action = 1;
    slpicpt(0, cmem, 1, &action, 2, &sigout, 2, &cntout, 2, &okout, 2);
    if (!okout) { printf("poc_shmem: slpicpt install failed\n"); exit(4); }

    /* build child param: "<block> <pid>\r" */
    sprintf(param, "%d %d", block, my_pid);
    plen = strlen(param);
    param[plen++] = '\r';

    /* child name "pocshmc" with high bit on last char */
    name[0] = 'p'; name[1] = 'o'; name[2] = 'c';
    name[3] = 's'; name[4] = 'h'; name[5] = 'm';
    name[6] = 'c' | 0x80;

    /* F$Fork: A=0 lang, B=namelen, X=name, Y=paramlen, U=param */
    r.rg_a = 0;
    r.rg_b = 7;
    r.rg_x = (unsigned)name;
    r.rg_y = plen;
    r.rg_u = (unsigned)param;
    if (_os9(F_FORK, &r)) {
        printf("poc_shmem: F$Fork err #%d\n", r.rg_b & 0xff);
        exit(5);
    }
    printf("poc_shmem: forked child pid=%d\n", r.rg_a & 0xff);

    /* poll for ack */
    sigout = 0;
    for (tries = 0; tries < POLL_TICKS; tries++) {
        action = 2;
        slpicpt(0, cmem, 1, &action, 2, &sigout, 2, &cntout, 2, &okout, 2);
        if (sigout == SIG_SHMEM_ACK) break;
        r.rg_x = 1;
        _os9(F_SLEEP, &r);
    }

    if (sigout == SIG_SHMEM_ACK) {
        printf("poc_shmem: ack sig=%d hits=%d (after %d ticks)\n",
               sigout, cntout, tries);
        printf("poc_shmem: shared magic=%x counter=%d (expected %x / 2)\n",
               (int)shared[0], (int)shared[1], MAGIC);
    } else {
        printf("poc_shmem: timeout last_sig=%d hits=%d\n", sigout, cntout);
    }

    /* reap child */
    if (_os9(F_WAIT, &r)) {
        printf("poc_shmem: F$Wait err #%d\n", r.rg_b & 0xff);
    } else {
        printf("poc_shmem: child exit=%d\n", r.rg_b & 0xff);
    }

    /* unmap block from our address space */
    r.rg_b = 1;
    r.rg_u = (unsigned)shared;
    if (_os9(F_CLRBLK, &r)) {
        printf("poc_shmem: F$ClrBlk err #%d\n", r.rg_b & 0xff);
    }

    /* free the physical block */
    r.rg_b = 1;
    r.rg_x = block;
    if (_os9(F_DELRAM, &r)) {
        printf("poc_shmem: F$DelRAM err #%d\n", r.rg_b & 0xff);
    }

    exit(0);
}
