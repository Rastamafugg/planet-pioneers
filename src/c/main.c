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

/* GameState in regular data segment. GDD §23.3 calls for `direct`
 * placement; deferred to a dedicated PoC -- see lessons-learned.md. */
GameState g_state;

static ginit()
{
    g_state.mode          = MODE_STANDARD;
    g_state.round         = 1;
    g_state.max_rounds    = 6;
    g_state.num_players   = 4;
    g_state.active_player = 0;
    g_state.phase         = PHASE_SUMMARY;
    printf("pp: init mode=%d players=%d max_rounds=%d\n",
           (int)g_state.mode,
           (int)g_state.num_players,
           (int)g_state.max_rounds);
}

static ph_sum() { printf("pp: r%d summary\n",      (int)g_state.round); }
static ph_lgr() { printf("pp: r%d land-grant\n",   (int)g_state.round); }
static ph_lau() { printf("pp: r%d land-auction\n", (int)g_state.round); }
static ph_rev() { printf("pp: r%d random-event\n", (int)g_state.round); }
static ph_mgt() { printf("pp: r%d management\n",   (int)g_state.round); }
static ph_prd() { printf("pp: r%d production\n",   (int)g_state.round); }
static ph_auc() { printf("pp: r%d auction\n",      (int)g_state.round); }

static nxt_ph()
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
    ginit();

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
        nxt_ph();
    }

    printf("pp: end after r%d\n", (int)(g_state.round - 1));
    exit(0);
}
