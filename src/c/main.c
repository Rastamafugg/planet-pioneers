/***********************************************************************
 * main.c  -  bisection step 2.
 *
 * Step 1 (struct + init + one multi-arg printf) ran clean. Add a
 * while loop with a single printf body, iterating 42 times (= 6
 * rounds * 7 phases). No switch yet -- this isolates "loop with many
 * printfs" from "switch dispatch".
 *
 * Compile: dcc main.c -m=4k -f=/dd/cmds/pioneer
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>

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
    g_state.mode          = 1;
    g_state.round         = 1;
    g_state.max_rounds    = 6;
    g_state.num_players   = 4;
    g_state.active_player = 0;
    g_state.phase         = 0;

    printf("pioneer: init mode=%d players=%d max_rounds=%d\n",
           g_state.mode, g_state.num_players, g_state.max_rounds);

    while (g_state.phase < 42) {
        printf("pioneer: tick %d\n", g_state.phase);
        g_state.phase++;
    }

    printf("pioneer: end\n");
    exit(0);
}
