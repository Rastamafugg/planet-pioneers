# Data Structures (DCC C)

From GDD §22. All structures use DCC's 16-bit int. Prefer `direct` storage class for frequently accessed globals (see [memory.md](../platform/memory.md)).

## Constants

```c
/* Commodity indices */
#define FOOD        0
#define ENERGY      1
#define SMITHORE    2
#define CRYSTITE    3
#define NUM_COMMODITIES 4

/* Terrain types */
#define TERRAIN_PLAINS   0
#define TERRAIN_RIVER    1
#define TERRAIN_MOUNTAIN 2

/* Map dimensions */
#define MAP_ROWS        5
#define MAP_COLS        9
#define STORE_COL       4    /* center column, not ownable */
#define MAX_PLOTS       44

/* Players and modes */
#define MAX_PLAYERS     4
#define MODE_BEGINNER   0
#define MODE_STANDARD   1
#define MODE_TOURNAMENT 2
#define MULE_NONE       -1

/* Auction minimums */
#define AUCTION_MIN_FOOD      15
#define AUCTION_MIN_ENERGY    10
#define AUCTION_MIN_SMITHORE  14
#define AUCTION_MIN_CRYSTITE  50
#define AUCTION_MAX_PRICE     265
```

## `Plot` — map cell state

```c
typedef struct {
    signed char owner;        /* -1=unowned, 0-3=player */
    unsigned char terrain;    /* TERRAIN_PLAINS/RIVER/MOUNTAIN */
    unsigned char mtn_count;  /* 0-3: mountain symbol count */
    signed char  mule_type;   /* MULE_NONE or FOOD/ENERGY/SMITHORE/CRYSTITE */
    unsigned char crystite;   /* 0-3: deposit level (Tournament) */
    unsigned char for_sale;   /* 0=no, 1=yes (Standard/Tournament) */
    unsigned int  sale_ask;   /* asking price if for_sale */
} Plot;

extern Plot g_map[MAP_ROWS][MAP_COLS];
```

~8 B per plot × 45 = ~360 B.

## `Player`

```c
typedef struct {
    unsigned int money;
    unsigned char goods[NUM_COMMODITIES];
    unsigned char color;      /* 0-3 */
    unsigned char species;    /* 0-7 */
    unsigned char is_human;
    unsigned char plot_count;
    unsigned int  net_worth;  /* updated each summary phase */
} Player;

extern Player g_players[MAX_PLAYERS];
```

~16 B × 4 = 64 B.

## `Store`

```c
typedef struct {
    unsigned char stock[NUM_COMMODITIES];
    unsigned int  price[NUM_COMMODITIES];
    unsigned char mule_count;
    unsigned int  mule_price;
    unsigned char smithore_queued;  /* last round's buy, builds next turn */
} Store;

extern Store g_store;
```

## `GameState` (direct page)

```c
typedef struct {
    unsigned char mode;
    unsigned char round;
    unsigned char max_rounds;      /* 6 or 12 */
    unsigned char num_players;
    unsigned char num_humans;
    unsigned char turn_order[MAX_PLAYERS];
    unsigned char active_player;
    unsigned char auction_commodity;
    unsigned int  crystite_price;  /* randomized per round (Tournament) */
} GameState;

direct GameState g_state;
```

## `Auction`

```c
typedef struct {
    unsigned char commodity;
    unsigned char is_buyer[MAX_PLAYERS];
    unsigned int  player_price[MAX_PLAYERS];
    unsigned char units_traded[MAX_PLAYERS];
    unsigned char declared[MAX_PLAYERS];
    unsigned int  store_price;
    unsigned char collusion_active;   /* Tournament */
    unsigned char collusion_mask;
} Auction;

extern Auction g_auction;
```

## `PlayerPos` — management-phase position

```c
typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char in_town;
    unsigned char mule_carried;  /* MULE_NONE or commodity index */
    unsigned int  time_ticks;
} PlayerPos;

extern PlayerPos g_pos[MAX_PLAYERS];
```

## `RandomEvent`

```c
typedef struct {
    unsigned char type;       /* 0=player, 1=planetary */
    unsigned char subtype;
    signed char   target;     /* player idx or -1 for all */
    int           magnitude;
} RandomEvent;
```

## `ProductionResult`

```c
typedef struct {
    unsigned char produced[MAX_PLAYERS][NUM_COMMODITIES];
    unsigned char spoiled[MAX_PLAYERS][NUM_COMMODITIES];
} ProductionResult;
```

## Static terrain tables

Place in **code segment** via `static const` to conserve data RAM:

```c
static const unsigned char terrain_map[MAP_ROWS][MAP_COLS] = { ... };
static const unsigned char mtn_count_map[MAP_ROWS][MAP_COLS] = { ... };
```

Full canonical values in [game/map.md](../game/map.md).

## Key function signatures (GDD §22.11)

- **Init:** `game_init`, `map_init`, `players_init`, `store_init`, `crystite_init`
- **Phases:** `phase_summary`, `phase_land_grant`, `phase_land_auction`, `phase_management(player)`, `phase_production(result)`, `phase_random_event(player)`, `phase_auction(commodity)`
- **Production:** `calc_base_production`, `calc_adjacency_bonus`, `calc_learning_bonus`, `calc_production`, `apply_spoilage`
- **Economy:** `update_store_prices`, `calc_net_worth`, `update_turn_order`, `calc_mule_price`
- **AI:** `ai_land_grant`, `ai_management`, `ai_auction_declare`, `ai_auction_move`
- **Render:** `draw_map`, `draw_town`, `draw_auction`, `draw_summary`, `draw_event`, `draw_production`
- **Input:** `read_joystick_x`, `read_joystick_y`, `read_button`
- **Sound:** `play_sound`, `play_production_unit`, `play_victory`

## Sources

- `docs/design/MULE_GDD.md` §22, §24
