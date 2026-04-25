/***********************************************************************
 * sound.c  -  Parent-side sound API (phase 3).
 *
 * Forks a `pocsndc` child that owns the blocking SS.Tone calls. The
 * parent's logic process produces {freq,dur,amp} tuples into a 16-
 * entry ring buffer in shared memory and signals the child.
 *
 * API:
 *   sound_init()                     -> 0 ok, non-zero err
 *   sound_play(freq, dur, amp)       -> 0 queued, -1 queue full
 *   sound_shutdown()                 -> 0 ok (waits for child to drain)
 *
 * SoundQueue layout MUST match poc_sndc.c.
 *
 * Compile (linked into the host): listed in `dcc <host>.c sound.c …`
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

static SoundQueue *g_queue;
static int         g_block;
static int         g_child_pid;

int sound_init()
{
    struct registers r;
    char  name[8];
    char  param[16];
    int   plen;

    g_queue = 0;
    g_block = -1;
    g_child_pid = -1;

    /* allocate one 8K block */
    r.rg_b = 1;
    if (_os9(F_ALLRAM, &r)) {
        printf("sound: F$AllRAM err #%d\n", r.rg_b & 0xff);
        return 1;
    }
    g_block = ((r.rg_a & 0xff) << 8) | (r.rg_b & 0xff);

    /* map into our DAT */
    r.rg_b = 1;
    r.rg_x = g_block;
    if (_os9(F_MAPBLK, &r)) {
        printf("sound: F$MapBlk err #%d\n", r.rg_b & 0xff);
        return 2;
    }
    g_queue = (SoundQueue *)r.rg_u;

    /* init queue */
    g_queue->magic = SOUND_MAGIC;
    g_queue->head  = 0;
    g_queue->tail  = 0;
    g_queue->quit  = 0;

    /* fork child */
    sprintf(param, "%d", g_block);
    plen = strlen(param);
    param[plen++] = '\r';

    name[0] = 'p'; name[1] = 'o'; name[2] = 'c';
    name[3] = 's'; name[4] = 'n'; name[5] = 'd';
    name[6] = 'c' | 0x80;

    r.rg_a = 0;
    r.rg_b = 7;
    r.rg_x = (unsigned)name;
    r.rg_y = plen;
    r.rg_u = (unsigned)param;
    if (_os9(F_FORK, &r)) {
        printf("sound: F$Fork err #%d\n", r.rg_b & 0xff);
        return 3;
    }
    g_child_pid = r.rg_a & 0xff;

    return 0;
}

int sound_play(freq, dur, amp)
int freq, dur, amp;
{
    int next;

    if (g_queue == 0) return -2;

    next = (g_queue->head + 1) & SOUND_QUEUE_MASK;
    if (next == g_queue->tail) return -1;            /* full */

    g_queue->entries[g_queue->head].freq = (unsigned)freq;
    g_queue->entries[g_queue->head].dur  = (unsigned)dur;
    g_queue->entries[g_queue->head].amp  = (unsigned)amp;
    g_queue->head = next;

    /* No F$Send: child polls on a short F$Sleep. Signals would
     * interrupt the child's blocking SS.Tone and abort tones early. */
    return 0;
}

int sound_shutdown()
{
    struct registers r;

    if (g_queue == 0) return 0;

    /* Tell child to drain remaining and exit. Child polls; will see
     * within ~33 ms. */
    g_queue->quit = 1;

    /* reap */
    if (_os9(F_WAIT, &r)) {
        printf("sound: F$Wait err #%d\n", r.rg_b & 0xff);
    }

    /* unmap and free */
    r.rg_b = 1;
    r.rg_u = (unsigned)g_queue;
    _os9(F_CLRBLK, &r);
    r.rg_b = 1;
    r.rg_x = g_block;
    _os9(F_DELRAM, &r);

    g_queue = 0;
    g_block = -1;
    g_child_pid = -1;
    return 0;
}
