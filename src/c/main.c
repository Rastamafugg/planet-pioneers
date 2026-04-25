/***********************************************************************
 * main.c  -  isolation bisect B.
 *
 * Bisect A (unsigned char fields + (int) casts) ran clean. So the
 * field type was NOT load-bearing. Suspects remaining: (B) the seven
 * static ph_xxx() helper functions, and (C) the short module name
 * `pp`. This step restores the helpers while keeping the module name
 * `pioneer`.
 *
 * If this crashes -> root cause is in how DCC compiles a static-fn
 * dispatch through a switch (likely codegen / linkage interaction).
 * If clean -> the only suspect left is the 2-char module name `pp`.
 *
 * Bisect C (this commit): output module renamed back to `pp`, all
 * other code unchanged from B. If this crashes -> root cause is
 * the short module name. If clean -> the original crash was a
 * combined effect we will need to revisit later (or a phantom).
 *
 * Compile: dcc main.c -m=4k -f=/dd/cmds/pp
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

main()
{
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
