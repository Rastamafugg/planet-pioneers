/***********************************************************************
 * score.c  -  Phase 7a: net-worth math + Summary / Colony-Failure /
 * Victory screens.
 *
 * Pure scoring (nw_calc, nw_combd, score_rk) plus three render-driving
 * functions (summshow / failshow / vicshow) that emit ren_clr -> a
 * batch of ren_text -> ren_pres -> ren_flush.
 *
 * 7a is a smoke test: real auction prices and real plot ownership
 * arrive in 7e/7f and 7b. Until then, score.c carries placeholder
 * starting prices and a small pl_seed() helper that gives each player
 * deterministic-but-distinct demo state so the screens have something
 * to show. Replace pl_seed() with real game init when 7b lands.
 *
 * DCC external-name discipline: K&R 8-char external significance, so
 * names like `summshow`/`failshow`/`vicshow` and `nw_calc`/`nw_combd`
 * are each unique within 8 chars (lessons-learned 2026-04-25).
 *
 * DCC varargs widening: every `unsigned char` passed to sprintf/printf
 * needs an explicit (int) cast at the call site
 * (lessons-learned 2026-04-25).
 *
 * Compile: linked into pioneer host:
 *   dcc /d1/main.c /d1/input.c /d1/render.c /d1/score.c -m=8k -f=...
 ***********************************************************************/
#include <stdio.h>
#include <string.h>

#define MAX_PLAYERS     4
#define NUM_COMMODITIES 4

#define FOOD     0
#define ENERGY   1
#define SMITHORE 2
#define CRYSTITE 3

#define MODE_BEGINNER   0
#define MODE_STANDARD   1
#define MODE_TOURNAMENT 2

#define LAND_VALUE 500
#define COLONY_SURVIVAL_THRESHOLD ((unsigned int)60000)

/* Screen geometry (matches poc_rndc.c SCR_W=160, SCR_H=192; 20x24 cells) */
#define COL_MAX 20
#define ROW_MAX 24

/* Palette indices for player rows. Default 16-color CoCo palette
 * already gives recognisably-distinct colors at 6/7/8/9; the proper
 * palette setup (GDD §19.2) lands when the game wires up palette
 * register init via the resolved R_OP_PALETTE escape path. */
#define COLOR_HEAD  15      /* white text for header / banner */
#define COLOR_DIM    7
static const unsigned char player_color[MAX_PLAYERS] = { 6, 7, 8, 9 };

/* Species names — short forms that fit the row layout. Indexed
 * by Player.species (0..7) per GDD §3.3. */
static char *species_name[8] = {
    "HUMAN",   /* expert tier */
    "MECH",
    "GOLLM",
    "BONZ",
    "LEGG",
    "PACK",
    "SPHR",
    "FLAP"     /* beginner tier */
};

/* Placeholder starting prices, used as last-auction-price fallback
 * until phase 7e/7f. Aligned with auction minimums (data-structures
 * AUCTION_MIN_*). Replace with shared price state when economy lands. */
static const unsigned int start_price[NUM_COMMODITIES] = {
    30,  /* food     */
    20,  /* energy   */
    50,  /* smithore */
    100  /* crystite (Tournament only) */
};

/* ---- Player table -------------------------------------------------- *
 * Pre-data-structures.md, full Player struct lives here. When 7b
 * introduces real ownership / cash flow, lift this to a shared header
 * so other phases see the same layout. */
typedef struct {
    unsigned int  money;
    unsigned char goods[NUM_COMMODITIES];
    unsigned char color;
    unsigned char species;
    unsigned char is_human;
    unsigned char plot_count;
    unsigned int  net_worth;
} Player;

Player g_players[MAX_PLAYERS];

/* GameState shared with main.c. Only `mode` and `round`/`max_rounds`
 * are read here. */
extern struct {
    unsigned char mode;
    unsigned char round;
    unsigned char max_rounds;
    unsigned char num_players;
    unsigned char active_player;
    unsigned char phase;
} g_state;

/* Render parent API. */
extern int ren_clr();
extern int ren_text();
extern int ren_pres();
extern int ren_flush();

/* ---- Pure scoring -------------------------------------------------- */

unsigned int nw_calc(p)
int p;
{
    Player *pl;
    unsigned int nw;
    int i, max;

    pl = &g_players[p];
    nw = pl->money;
    max = (g_state.mode == MODE_TOURNAMENT) ? NUM_COMMODITIES
                                            : (NUM_COMMODITIES - 1);
    for (i = 0; i < max; i++) {
        nw += (unsigned int)pl->goods[i] * start_price[i];
    }
    nw += (unsigned int)pl->plot_count * LAND_VALUE;

    pl->net_worth = nw;
    return nw;
}

unsigned int nw_combd()
{
    unsigned int sum;
    int p;
    sum = 0;
    for (p = 0; p < (int)g_state.num_players; p++) sum += nw_calc(p);
    return sum;
}

/* score_rk: fill order[0..num_players-1] with player indices sorted by
 * net_worth descending; ties broken by lower index first. Caller must
 * have computed net_worth (call nw_combd first, or nw_calc each).
 * Insertion sort — at most 4 entries. */
score_rk(order)
unsigned char *order;
{
    int n, i, j, p0, p1;
    unsigned char tmp;
    unsigned int a, b;

    n = (int)g_state.num_players;
    for (i = 0; i < n; i++) order[i] = (unsigned char)i;

    for (i = 1; i < n; i++) {
        for (j = i; j > 0; j--) {
            p0 = (int)order[j - 1];
            p1 = (int)order[j];
            a = g_players[p0].net_worth;
            b = g_players[p1].net_worth;
            if (b > a || (b == a && p1 < p0)) {
                tmp = order[j - 1];
                order[j - 1] = order[j];
                order[j] = tmp;
            } else {
                break;
            }
        }
    }
}

/* ---- Demo seed (placeholder until 7b) ----------------------------- */

pl_seed()
{
    int i;
    static unsigned int seed_money[MAX_PLAYERS] = {1000, 1200, 800, 1500};
    static unsigned char seed_food[MAX_PLAYERS] = {4, 2, 6, 3};
    static unsigned char seed_enrg[MAX_PLAYERS] = {2, 3, 1, 4};
    static unsigned char seed_smit[MAX_PLAYERS] = {0, 1, 2, 0};
    static unsigned char seed_plot[MAX_PLAYERS] = {2, 1, 3, 2};
    static unsigned char seed_spec[MAX_PLAYERS] = {0, 1, 2, 7};

    for (i = 0; i < MAX_PLAYERS; i++) {
        g_players[i].money = seed_money[i];
        g_players[i].goods[FOOD]     = seed_food[i];
        g_players[i].goods[ENERGY]   = seed_enrg[i];
        g_players[i].goods[SMITHORE] = seed_smit[i];
        g_players[i].goods[CRYSTITE] = 0;
        g_players[i].color      = player_color[i];
        g_players[i].species    = seed_spec[i];
        g_players[i].is_human   = (i == 0);
        g_players[i].plot_count = seed_plot[i];
        g_players[i].net_worth  = 0;
    }
}

/* ---- Screen helpers ----------------------------------------------- */

/* Place text centered horizontally in the 20-col grid. `len` chars wide. */
static center_col(len)
int len;
{
    int c;
    c = (COL_MAX - len) / 2;
    if (c < 0) c = 0;
    return c;
}

static char *rank_label(r)
int r;
{
    static char *labels[4] = { "1ST", "2ND", "3RD", "4TH" };
    if (r < 0 || r > 3) return "   ";
    return labels[r];
}

/* draw_player_row: rank, species, net worth, plot count.
 *
 * 20-col layout:
 *   col 0  : rank label "1ST"
 *   col 4  : species (up to 5 chars)
 *   col 11 : "$" + net worth (right-justified, up to 5 digits)
 *   col 17 : plots "##P"
 */
static draw_player_row(rank, p, row)
int rank, p, row;
{
    char buf[16];
    int color;

    color = (int)g_players[p].color;

    ren_text(0, row, color, rank_label(rank));
    ren_text(4, row, color, species_name[g_players[p].species & 7]);

    sprintf(buf, "$%u", g_players[p].net_worth);
    ren_text(11, row, color, buf);

    sprintf(buf, "%dP", (int)g_players[p].plot_count);
    ren_text(17, row, color, buf);
}

/* ---- Summary screen ----------------------------------------------- */

summshow()
{
    char hdr[24];
    unsigned char order[MAX_PLAYERS];
    int i, n, col, row;

    nw_combd();              /* refresh net_worth on every player */
    score_rk(order);

    ren_clr(0);              /* black background */

    sprintf(hdr, "ROUND %d OF %d",
            (int)g_state.round, (int)g_state.max_rounds);
    col = center_col((int)strlen(hdr));
    ren_text(col, 1, COLOR_HEAD, hdr);

    n = (int)g_state.num_players;
    row = 5;
    for (i = 0; i < n; i++) {
        draw_player_row(i, (int)order[i], row);
        row += 3;
    }

    ren_pres();
    ren_flush();
}

/* ---- Colony Failure screen ---------------------------------------- */

failshow()
{
    char buf[24];
    unsigned int total;
    int i, col;

    total = nw_combd();

    ren_clr(0);

    ren_text(center_col(13), 2, COLOR_HEAD, "COLONY FAILED");
    sprintf(buf, "$%u / $60000", total);
    col = center_col((int)strlen(buf));
    ren_text(col, 5, COLOR_HEAD, buf);

    /* Loser roster — no rank, just identity + nw, in player order. */
    for (i = 0; i < (int)g_state.num_players; i++) {
        sprintf(buf, "%s $%u",
                species_name[g_players[i].species & 7],
                g_players[i].net_worth);
        ren_text(2, 9 + i * 2, (int)g_players[i].color, buf);
    }

    ren_pres();
    ren_flush();
}

/* ---- Victory screen ------------------------------------------------ */

vicshow()
{
    char buf[24];
    unsigned char order[MAX_PLAYERS];
    int i, n, ties, col;
    unsigned int top;

    nw_combd();
    score_rk(order);

    /* Count co-winners (everyone tied with order[0] at the top). */
    n = (int)g_state.num_players;
    top = g_players[order[0]].net_worth;
    ties = 1;
    for (i = 1; i < n; i++) {
        if (g_players[order[i]].net_worth == top) ties++;
        else break;
    }

    ren_clr(0);

    if (ties > 1) {
        ren_text(center_col(14), 1, COLOR_HEAD, "FIRST FOUNDERS");
    } else {
        sprintf(buf, "FIRST FOUNDER:");
        col = center_col((int)strlen(buf));
        ren_text(col, 1, COLOR_HEAD, buf);
        col = center_col((int)strlen(species_name[
                          g_players[order[0]].species & 7]));
        ren_text(col, 3, (int)g_players[order[0]].color,
                 species_name[g_players[order[0]].species & 7]);
    }

    /* Final leaderboard. */
    for (i = 0; i < n; i++) {
        sprintf(buf, "%s %s $%u",
                rank_label(i),
                species_name[g_players[order[i]].species & 7],
                g_players[order[i]].net_worth);
        ren_text(1, 7 + i * 2, (int)g_players[order[i]].color, buf);
    }

    ren_pres();
    ren_flush();
}
