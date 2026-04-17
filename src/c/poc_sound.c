/***********************************************************************
 * poc_sound.c  -  NitrOS-9 EOU / CoCo 3  Sound PoC
 *
 * Verifies:
 *   - SS.Tone (I$SetStt code 0x98) on /term path
 *   - Note frequency table (values require hardware calibration)
 *   - Melody playback: sequential blocking tone calls
 *   - Two SFX sequences (collect resource, MULE escape)
 *
 * CRITICAL: SS.Tone is BLOCKING. Process pauses for full duration.
 * Production architecture requires a dedicated sound process that
 * reads {freq, dur, amp} tuples from a pipe (coordinator -> sound).
 * This PoC is single-process only.
 *
 * Frequency values (N_*): 0-4095 relative scale, higher = higher pitch.
 * These are ESTIMATED. Calibrate by ear on actual CoCo 3 hardware.
 * The VTIO/SndDrv timer divisor determines the actual Hz mapping.
 *
 * Compile:  dcc poc_sound.c -o=poc_sound
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#define I_SETSTT  0x8E
#define F_SLEEP   0x0A
#define SS_TONE   0x98        /* SS.Tone SetStat code                 */

/*
 * Note frequency table — REQUIRES HARDWARE CALIBRATION.
 * Starting estimates for one octave (C4-C5).
 * Adjust by playing N_A4 and comparing to 440 Hz reference.
 */
#define N_C4    600
#define N_D4    675
#define N_E4    755
#define N_F4    800
#define N_G4    900
#define N_A4   1010
#define N_B4   1135
#define N_C5   1200
#define N_REST    0           /* rest: F$Sleep instead of SS.Tone    */

#define AMP_MUS  28           /* music amplitude (0-63)              */
#define AMP_SFX  48           /* SFX amplitude                       */

int open(), close();

static int g_snd;             /* path for SS.Tone                    */

/*
 * Play one tone.
 * freq=N_REST: sleep for dur ticks instead.
 * dur: duration in ticks (1/60 s each), range 1-255.
 * amp: volume 0-63.
 *
 * SS.Tone X register: MSB = amp (0-63), LSB = dur (ticks).
 * SS.Tone Y register: frequency 0-4095.
 */
void play_tone(freq, dur, amp)
int freq, dur, amp;
{
    struct registers r;

    if (freq == N_REST) {
        r.rg_x = (unsigned)dur;
        _os9(F_SLEEP, &r);
        return;
    }
    r.rg_a = (char)g_snd;
    r.rg_b = (char)SS_TONE;
    r.rg_x = (unsigned)(((amp & 0x3F) << 8) | (dur & 0xFF));
    r.rg_y = (unsigned)(freq & 0x0FFF);
    _os9(I_SETSTT, &r);
}

/*
 * Sequence format: flat int array of {freq, dur} pairs,
 * terminated by {-1, 0}.
 */
void play_seq(seq, amp)
int *seq; int amp;
{
    while (seq[0] != -1) {
        play_tone(seq[0], seq[1], amp);
        seq += 2;
    }
}

/* ------------------------------------------------------------------
 * Melody: simple M.U.L.E.-style motif
 * ------------------------------------------------------------------ */
static int melody[] = {
    N_C4,  8,   N_E4,  6,   N_G4,  6,
    N_C5, 14,   N_REST, 4,
    N_G4,  8,   N_E4,  6,   N_C4, 12,
    N_REST, 6,
    N_E4,  6,   N_G4,  6,   N_A4,  6,
    N_G4, 14,   N_REST, 4,
    N_C4,  6,   N_E4,  6,   N_G4,  6,   N_C5, 20,
    N_REST, 8,
    -1, 0
};

/* Resource collected: ascending arpeggio */
static int sfx_collect[] = {
    N_G4, 3,  N_A4, 3,  N_C5, 5,
    -1, 0
};

/* MULE escaped: descending drop */
static int sfx_escape[] = {
    N_C5, 3,  N_G4, 4,  N_E4, 4,  N_C4, 8,
    -1, 0
};

/* Production credit jingle: fast arpeggio per unit produced */
static int sfx_produce[] = {
    N_E4, 2,  N_G4, 2,  N_C5, 3,
    -1, 0
};

main()
{
    struct registers r;

    /* SS.Tone can target any window or /term path */
    g_snd = open("/term", 3);
    if (g_snd < 0) {
        fprintf(stderr, "poc_sound: cannot open /term\n");
        exit(1);
    }

    printf("poc_sound: melody\n");
    play_seq(melody, AMP_MUS);

    /* Pause between sequences */
    r.rg_x = 30; _os9(F_SLEEP, &r);

    printf("poc_sound: collect SFX\n");
    play_seq(sfx_collect, AMP_SFX);

    r.rg_x = 15; _os9(F_SLEEP, &r);

    printf("poc_sound: escape SFX\n");
    play_seq(sfx_escape, AMP_SFX);

    r.rg_x = 15; _os9(F_SLEEP, &r);

    printf("poc_sound: produce SFX (5x)\n");
    {
        int i;
        for (i = 0; i < 5; i++) {
            play_seq(sfx_produce, AMP_SFX);
            r.rg_x = 8; _os9(F_SLEEP, &r);
        }
    }

    printf("poc_sound: done\n");
    close(g_snd);
    exit(0);
}