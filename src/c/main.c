/***********************************************************************
 * main.c  -  Planet Pioneers core skeleton (phase 1).
 *
 * Bisect step 3: restore switch dispatch + round/phase advance over
 * a 6-round x 7-phase game loop. Steps 1 and 2 (struct + init + one
 * printf, then 42-iteration loop with a single printf body) both
 * ran clean.
 *
 * Compile: dcc main.c -m=4k -f=/dd/cmds/pioneer
 *
 * See wiki/implementation/roadmap.md (phase 1).
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>

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
    int mode;
    int round;
    int max_rounds;
    int num_players;
    int active_player;
    int phase;
} GameState;

GameState g_state;

main()
{
    g_state.mode          = MODE_STANDARD;
    g_state.round         = 1;
    g_state.max_rounds    = 6;
    g_state.num_players   = 4;
    g_state.active_player = 0;
    g_state.phase         = PHASE_SUMMARY;

    printf("pioneer: init mode=%d players=%d max_rounds=%d\n",
           g_state.mode, g_state.num_players, g_state.max_rounds);

    while (g_state.phase != PHASE_END) {
        switch (g_state.phase) {
        case PHASE_SUMMARY:
            printf("pioneer: r%d summary\n", g_state.round);
            break;
        case PHASE_LAND_GRANT:
            printf("pioneer: r%d land-grant\n", g_state.round);
            break;
        case PHASE_LAND_AUCTION:
            printf("pioneer: r%d land-auction\n", g_state.round);
            break;
        case PHASE_RANDOM_EVENT:
            printf("pioneer: r%d random-event\n", g_state.round);
            break;
        case PHASE_MANAGEMENT:
            printf("pioneer: r%d management\n", g_state.round);
            break;
        case PHASE_PRODUCTION:
            printf("pioneer: r%d production\n", g_state.round);
            break;
        case PHASE_AUCTION:
            printf("pioneer: r%d auction\n", g_state.round);
            break;
        }

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

    printf("pioneer: end after r%d\n", g_state.round - 1);
    exit(0);
}
