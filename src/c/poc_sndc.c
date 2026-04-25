/***********************************************************************
 * poc_sndc.c  -  Sound child process (phase 3).
 *
 * Maps the shared SoundQueue from the block# in argv[1], opens /term
 * for SS.Tone, then sleeps until signalled. On each wake it drains
 * every queued tone via SS.Tone (which BLOCKS for the tone duration)
 * and goes back to sleep. When q->quit is set, drains any remaining
 * entries and exits cleanly.
 *
 * SoundQueue layout MUST match sound.c.
 *
 * Compile: dcc poc_sndc.c -m=4k -f=/dd/cmds/pocsndc
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef F_SLEEP
#define F_SLEEP  0x0A
#endif
#ifndef F_MAPBLK
#define F_MAPBLK 0x3D
#endif
#ifndef F_CLRBLK
#define F_CLRBLK 0x38
#endif
#ifndef I_SETSTT
#define I_SETSTT 0x8E
#endif
#ifndef SS_TONE
#define SS_TONE  0x98
#endif

#define SOUND_MAGIC      0x534E
#define SOUND_QUEUE_SIZE 16
#define SOUND_QUEUE_MASK (SOUND_QUEUE_SIZE - 1)

typedef struct {
    unsigned int freq;
    unsigned int dur;
    unsigned int amp;
} SoundCmd;

typedef struct {
    unsigned int magic;
    unsigned int head;
    unsigned int tail;
    unsigned int quit;
    SoundCmd     entries[SOUND_QUEUE_SIZE];
} SoundQueue;

extern int open(), close();

main(argc, argv)
int argc;
char *argv[];
{
    struct registers r;
    SoundQueue *q;
    int   block;
    int   term;
    int   idx;
    int   freq, dur, amp;
    int   err;

    if (argc < 2) { printf("pocsndc: need block#\n"); exit(1); }
    block = atoi(argv[1]);
    printf("pocsndc: started block=%d\n", block);

    /* map shared block */
    r.rg_b = 1;
    r.rg_x = block;
    if (_os9(F_MAPBLK, &r)) {
        printf("pocsndc: F$MapBlk b=%d err #%d\n",
               block, r.rg_b & 0xff);
        exit(2);
    }
    q = (SoundQueue *)r.rg_u;

    if ((int)q->magic != SOUND_MAGIC) {
        printf("pocsndc: bad magic %x\n", (int)q->magic);
        exit(3);
    }

    /* open /term mode 3 (r/w) per poc_sound.c -- write-only fails silently */
    term = open("/term", 3);
    if (term < 0) { printf("pocsndc: open /term failed\n"); exit(4); }
    printf("pocsndc: term=%d, draining\n", term);

    /* Poll-based drain loop. Short F$Sleep instead of signal-as-wakeup
     * because SS.Tone is interruptible by signals -- the parent's
     * signals (one per sound_play, plus shutdown) would otherwise abort
     * each tone almost immediately. */
    for (;;) {
        while (q->tail != q->head) {
            idx  = q->tail;
            freq = (int)q->entries[idx].freq;
            dur  = (int)q->entries[idx].dur;
            amp  = (int)q->entries[idx].amp;
            printf("pocsndc: tone f=%d d=%d a=%d\n", freq, dur, amp);

            r.rg_a = (char)term;
            r.rg_b = (char)SS_TONE;
            r.rg_x = ((amp & 0x3F) << 8) | (dur & 0xFF);
            r.rg_y = freq & 0x0FFF;
            err = _os9(I_SETSTT, &r);
            if (err) {
                printf("pocsndc: SS.Tone err #%d\n", r.rg_b & 0xff);
            }

            q->tail = (q->tail + 1) & SOUND_QUEUE_MASK;
        }
        if (q->quit) break;

        /* short poll -- 2 ticks = ~33 ms */
        r.rg_x = 2;
        _os9(F_SLEEP, &r);
    }
    printf("pocsndc: exiting\n");

    close(term);

    /* unmap our copy; parent F$DelRAMs */
    r.rg_b = 1;
    r.rg_u = (unsigned)q;
    _os9(F_CLRBLK, &r);

    exit(0);
}
