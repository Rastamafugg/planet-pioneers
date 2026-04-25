/***********************************************************************
 * main.c  -  bisection step 1.
 *
 * Smallest possible extension over poc_hello: a global struct, init,
 * and one multi-arg printf. No loop, no switch yet.
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
    g_state.phase         = 1;

    printf("pioneer: init mode=%d players=%d max_rounds=%d\n",
           g_state.mode, g_state.num_players, g_state.max_rounds);

    exit(0);
}
