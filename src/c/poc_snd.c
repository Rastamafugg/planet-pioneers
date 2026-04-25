/***********************************************************************
 * poc_snd.c  -  Sound API driver (phase 3 PoC parent).
 *
 * Minimal smoke test: bring up the sound child, queue four tones in
 * fire-and-forget fashion, shut down. Parent must NOT block while
 * each tone plays -- the queue + child handle the blocking SS.Tone.
 *
 * Compile: dcc poc_snd.c sound.c -m=4k -f=/dd/cmds/pocsnd
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>

extern int sound_init();
extern int sound_play();
extern int sound_shutdown();

main()
{
    int err;

    err = sound_init();
    if (err) { printf("poc_snd: sound_init err %d\n", err); exit(1); }

    printf("poc_snd: queueing 4 tones\n");
    sound_play(2900, 30, 52);  /* C5  AMP_LOUD */
    sound_play(2440, 30, 52);  /* A4 */
    sound_play(1825, 30, 52);  /* E4 */
    sound_play(1450, 60, 52);  /* C4, longer */

    printf("poc_snd: queued; shutting down (blocks until drained)\n");
    sound_shutdown();
    printf("poc_snd: clean\n");
    exit(0);
}
