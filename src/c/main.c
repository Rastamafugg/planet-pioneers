/***********************************************************************
 * main.c  -  isolation bisect A.
 *
 * Working baseline = int fields + inlined printfs + module name
 * `pioneer`. This step reverts ONLY the field types to unsigned char
 * (with explicit (int) casts at every printf site) -- the same
 * pattern that crashed in PR #8. Everything else stays at the
 * working baseline.
 *
 * If this crashes -> root cause is the unsigned-char-through-printf
 * pattern, even with (int) casts; the cast does not actually save
 * us under DCC's K&R varargs ABI.
 *
 * If this runs clean -> field type was NOT the load-bearing change;
 * move on to bisect B (revert the helper functions).
 *
 * Compile: dcc main.c -m=4k -f=/dd/cmds/pioneer
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
        case PHASE_SUMMARY:
            printf("pioneer: r%d summary\n", (int)g_state.round);
            break;
        case PHASE_LAND_GRANT:
            printf("pioneer: r%d land-grant\n", (int)g_state.round);
            break;
        case PHASE_LAND_AUCTION:
            printf("pioneer: r%d land-auction\n", (int)g_state.round);
            break;
        case PHASE_RANDOM_EVENT:
            printf("pioneer: r%d random-event\n", (int)g_state.round);
            break;
        case PHASE_MANAGEMENT:
            printf("pioneer: r%d management\n", (int)g_state.round);
            break;
        case PHASE_PRODUCTION:
            printf("pioneer: r%d production\n", (int)g_state.round);
            break;
        case PHASE_AUCTION:
            printf("pioneer: r%d auction\n", (int)g_state.round);
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

    printf("pioneer: end after r%d\n", (int)(g_state.round - 1));
    exit(0);
}
