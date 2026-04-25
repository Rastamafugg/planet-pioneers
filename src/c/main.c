/***********************************************************************
 * main.c  -  Planet Pioneers core skeleton (phase 1)
 *
 * Minimal turn-phase state machine. Phase functions are stubs that
 * print their name; loop advances through the phase list and
 * increments g_state.round until max_rounds is reached.
 *
 * Compile: dcc main.c -m=4k -f=/dd/cmds/pp
 *
 * See wiki/implementation/roadmap.md (phase 1).
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define MAX_PLAYERS     4

#define MODE_BEGINNER   0
#define MODE_STANDARD   1
#define MODE_TOURNAMENT 2

#define PHASE_INIT          0
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

direct GameState g_state;

game_init()
{
    g_state.mode          = MODE_STANDARD;
    g_state.round         = 1;
    g_state.max_rounds    = 6;
    g_state.num_players   = 4;
    g_state.active_player = 0;
    g_state.phase         = PHASE_SUMMARY;
    printf("pp: init mode=%d players=%d max_rounds=%d\n",
           g_state.mode, g_state.num_players, g_state.max_rounds);
}

phase_summary()       { printf("pp: r%d summary\n",       g_state.round); }
phase_land_grant()    { printf("pp: r%d land-grant\n",    g_state.round); }
phase_land_auction()  { printf("pp: r%d land-auction\n",  g_state.round); }
phase_random_event()  { printf("pp: r%d random-event\n",  g_state.round); }
phase_management()    { printf("pp: r%d management\n",    g_state.round); }
phase_production()    { printf("pp: r%d production\n",    g_state.round); }
phase_auction()       { printf("pp: r%d auction\n",       g_state.round); }

next_phase()
{
    if (g_state.phase >= PHASE_AUCTION) {
        g_state.round++;
        if (g_state.round > g_state.max_rounds) {
            g_state.phase = PHASE_END;
            return;
        }
        g_state.phase = PHASE_SUMMARY;
        return;
    }
    g_state.phase++;
}

main()
{
    game_init();

    while (g_state.phase != PHASE_END) {
        switch (g_state.phase) {
        case PHASE_SUMMARY:      phase_summary();      break;
        case PHASE_LAND_GRANT:   phase_land_grant();   break;
        case PHASE_LAND_AUCTION: phase_land_auction(); break;
        case PHASE_RANDOM_EVENT: phase_random_event(); break;
        case PHASE_MANAGEMENT:   phase_management();   break;
        case PHASE_PRODUCTION:   phase_production();   break;
        case PHASE_AUCTION:      phase_auction();      break;
        }
        next_phase();
    }

    printf("pp: end after r%d\n", g_state.round - 1);
    exit(0);
}
