/***********************************************************************
 * poc_sound.c  -  NitrOS-9 EOU / CoCo 3 Sound PoC
 *
 * Verifies:
 *   - SS.Tone (I$SetStt code 0x98) on /term path
 *   - Higher relative frequency ranges for more audible pitch spread
 *   - Melody playback, amplitude-accented sequences, sweeps, and warbles
 *
 * CRITICAL: SS.Tone is BLOCKING. Process pauses for full duration.
 * Production architecture requires a dedicated sound process that
 * reads {freq, dur, amp} tuples from a pipe.
 *
 * Frequency values are relative counters from 0-4095, not Hz.
 * The NitrOS-9 reference notes that the widest tone variation occurs
 * at the high range, so this test intentionally uses high values.
 *
 * Compile: dcc poc_sound.c -s -f=/dd/cmds/pocsound
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef I_SETSTT
#define I_SETSTT  0x8E
#endif
#ifndef F_SLEEP
#define F_SLEEP   0x0A
#endif
#ifndef SS_TONE
#define SS_TONE   0x98
#endif

/*
 * Relative pitch table for SS.Tone.
 * These values are perceptual test points, not musical note frequencies.
 */
#define N_C4   1450
#define N_D4   1625
#define N_E4   1825
#define N_F4   1935
#define N_G4   2175
#define N_A4   2440
#define N_B4   2740
#define N_C5   2900
#define N_D5   3250
#define N_E5   3650
#define N_FX   4050
#define N_REST    0

#define AMP_SOFT 18
#define AMP_MUS  32
#define AMP_LOUD 52
#define AMP_SFX  58

int open(), close();

static int g_snd;

play_tone(freq, dur, amp)
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

play_seq(seq, amp)
int *seq; int amp;
{
    while (seq[0] != -1) {
        play_tone(seq[0], seq[1], amp);
        seq += 2;
    }
}

play_seq_amp(seq)
int *seq;
{
    while (seq[0] != -1) {
        play_tone(seq[0], seq[1], seq[2]);
        seq += 3;
    }
}

pitch_sweep(start, stop, step, dur, amp)
int start, stop, step, dur, amp;
{
    int f;

    if (step == 0) return;

    if (start <= stop) {
        for (f = start; f <= stop; f += step) play_tone(f, dur, amp);
    } else {
        for (f = start; f >= stop; f -= step) play_tone(f, dur, amp);
    }
}

warble(base, spread, count, dur, amp)
int base, spread, count, dur, amp;
{
    int i;

    for (i = 0; i < count; i++) {
        play_tone(base - spread, dur, amp);
        play_tone(base + spread, dur, amp);
    }
}

static int melody[] = {
    N_C4,  6,   N_E4,  5,   N_G4,  5,
    N_C5, 10,   N_D5,  4,   N_C5,  8,
    N_REST, 3,
    N_G4,  5,   N_B4,  5,   N_D5,  8,
    N_E5, 10,   N_REST, 3,
    N_E4,  4,   N_G4,  4,   N_A4,  4,
    N_C5,  8,   N_A4,  4,   N_G4, 10,
    N_REST, 4,
    N_C5,  4,   N_D5,  4,   N_E5,  4,
    N_FX,  6,   N_E5,  6,   N_C5, 14,
    -1, 0
};

static int fanfare[] = {
    N_C5, 3, AMP_MUS,   N_E5, 3, AMP_LOUD,
    N_G4, 3, AMP_MUS,   N_D5, 3, AMP_LOUD,
    N_C5, 6, AMP_SOFT,  N_FX, 8, AMP_LOUD,
    N_REST, 3, AMP_SOFT,
    -1, 0, 0
};

static int sfx_collect[] = {
    N_C5, 2,  N_E5, 2,  N_FX, 4,
    N_REST, 1,
    N_D5, 2,  N_FX, 3,
    -1, 0
};

static int sfx_escape[] = {
    N_FX, 2,  N_E5, 2,  N_D5, 2,  N_C5, 3,
    N_A4, 3,  N_F4, 4,  N_D4, 5,  N_C4, 10,
    -1, 0
};

static int sfx_produce[] = {
    N_E5, 2,  N_C5, 2,  N_G4, 2,  N_D5, 3,
    -1, 0
};

main()
{
    struct registers r;
    int i;

    g_snd = open("/term", 3);
    if (g_snd < 0) {
        fprintf(stderr, "poc_sound: cannot open /term\n");
        exit(1);
    }

    printf("poc_sound: melody\n");
    play_seq(melody, AMP_MUS);

    r.rg_x = 30; _os9(F_SLEEP, &r);

    printf("poc_sound: fanfare\n");
    play_seq_amp(fanfare);

    r.rg_x = 15; _os9(F_SLEEP, &r);

    printf("poc_sound: collect SFX\n");
    play_seq(sfx_collect, AMP_SFX);
    pitch_sweep(2600, 4050, 180, 1, AMP_LOUD);

    r.rg_x = 15; _os9(F_SLEEP, &r);

    printf("poc_sound: escape SFX\n");
    play_seq(sfx_escape, AMP_SFX);
    pitch_sweep(4050, 1250, 260, 2, AMP_SFX);

    r.rg_x = 15; _os9(F_SLEEP, &r);

    printf("poc_sound: claim jump warning\n");
    warble(3050, 430, 8, 2, AMP_LOUD);

    r.rg_x = 15; _os9(F_SLEEP, &r);

    printf("poc_sound: produce SFX (5x)\n");
    for (i = 0; i < 5; i++) {
        play_seq(sfx_produce, AMP_SFX);
        pitch_sweep(2300 + i * 150, 3100 + i * 150, 200, 1, AMP_MUS);
        r.rg_x = 8; _os9(F_SLEEP, &r);
    }

    printf("poc_sound: high range sweep\n");
    pitch_sweep(1200, 4050, 160, 1, AMP_SOFT);
    pitch_sweep(4050, 1600, 220, 1, AMP_LOUD);

    printf("poc_sound: done\n");
    close(g_snd);
    exit(0);
}
