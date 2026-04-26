/***********************************************************************
 * main.c  -  Planet Pioneers core skeleton (phase 1).
 *
 * Turn-phase state machine. Stub phase functions print their name
 * and return; main() advances g_state.phase / g_state.round until
 * max_rounds is reached.
 *
 * GameState fields are unsigned char per data-structures.md. Pass
 * with (int) casts at every printf site -- DCC's K&R varargs ABI
 * requires the call-site widening (see lessons-learned.md).
 *
 * Compile: dcc main.c input.c -m=4k -f=/dd/cmds/pioneer
 *
 * See wiki/implementation/roadmap.md (phase 1, 5).
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <os9.h>

#ifndef F_SLEEP
#define F_SLEEP 0x0A
#endif

#define K_SPACE 0x80
#define K_CTRL  0x02

extern int inp_init();
extern int inp_poll();
extern int inp_pres();
extern int inp_held();

#define MODE_BEGINNER   0
#define MODE_STANDARD   1
#define MODE_TOURNAMENT 2

#define PHASE_SUMMARY       1
#define PHASE_LAND_GRANT    2
#define PHASE_LAND_AUCTION  3
#define PHASE_RANDOM_EVENT  4
#define PHASE_MANAGEMENT    5
#define PHASE_PRODUCTION    6
#define PHASE_AUCTION       7
#define PHASE_END           8

typedef struct {
    unsigned char mode;
    unsigned char round;
    unsigned char max_rounds;
    unsigned char num_players;
    unsigned char active_player;
    unsigned char phase;
} GameState;

GameState g_state;

static ph_sum() { printf("pioneer: r%d summary\n",      (int)g_state.round); }
static ph_lgr() { printf("pioneer: r%d land-grant\n",   (int)g_state.round); }
static ph_lau() { printf("pioneer: r%d land-auction\n", (int)g_state.round); }
static ph_rev() { printf("pioneer: r%d random-event\n", (int)g_state.round); }
static ph_mgt() { printf("pioneer: r%d management\n",   (int)g_state.round); }
static ph_prd() { printf("pioneer: r%d production\n",   (int)g_state.round); }
static ph_auc() { printf("pioneer: r%d auction\n",      (int)g_state.round); }

/* Block until SPACE pressed. CTRL+SPACE forces fast exit (skip-to-end).
 * Polls SS.KySns at ~20 Hz (3-tick sleep) -- responsive but cheap. */
static gate()
{
    struct registers r;

    inp_poll();  /* clear pending edge from prior phase */
    for (;;) {
        r.rg_x = 3;
        _os9(F_SLEEP, &r);
        inp_poll();
        if (inp_pres(K_SPACE)) {
            if (inp_held(K_CTRL)) g_state.round = g_state.max_rounds;
            return;
        }
    }
}

main()
{
    if (inp_init()) { printf("pioneer: inp_init failed\n"); exit(1); }

    g_state.mode          = MODE_STANDARD;
    g_state.round         = 1;
    g_state.max_rounds    = 6;
    g_state.num_players   = 4;
    g_state.active_player = 0;
    g_state.phase         = PHASE_SUMMARY;

    printf("pioneer: init mode=%d players=%d max_rounds=%d\n",
           (int)g_state.mode,
           (int)g_state.num_players,
           (int)g_state.max_rounds);
    printf("pioneer: SPACE advances; CTRL+SPACE skips to end\n");

    while (g_state.phase != PHASE_END) {
        switch (g_state.phase) {
        case PHASE_SUMMARY:      ph_sum(); break;
        case PHASE_LAND_GRANT:   ph_lgr(); break;
        case PHASE_LAND_AUCTION: ph_lau(); break;
        case PHASE_RANDOM_EVENT: ph_rev(); break;
        case PHASE_MANAGEMENT:   ph_mgt(); break;
        case PHASE_PRODUCTION:   ph_prd(); break;
        case PHASE_AUCTION:      ph_auc(); break;
        }

        gate();

        if (g_state.phase >= PHASE_AUCTION) {
            g_state.round++;
            if (g_state.round > g_state.max_rounds) {
                g_state.phase = PHASE_END;
            } else {
                g_state.phase = PHASE_SUMMARY;
            }
        } else {
            g_state.phase++;
        }
    }

    printf("pioneer: end after r%d\n", (int)(g_state.round - 1));
    exit(0);
}
