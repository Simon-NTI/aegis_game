#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define SKYBLUE_LIGHT \
    (Color) { .r = 162, .g = 215, .b = 255, .a = 255 }

float g_frame_time = 0;
Vector2 g_mouse_position = {0};

struct
{
    int width;
    const int height;
} g_window = {.width = 600, .height = 800};

typedef enum
{
    STATE_MAIN_MENU,
    STATE_PAUSED,
    STATE_LEVEL_SELECTION,
    STATE_LEVEL,
    STATE_UPGRADE,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_COUNT
} Gamestate_e;

typedef struct
{
    Vector2 position;
    float radius;
    unsigned char color_index;
    unsigned char speed_index;
    float twinkle_offset;
} Star_t;

struct
{
    int star_count;
    Star_t stars[200];
    unsigned char speeds_count;
    Vector2 star_speeds[3];
    unsigned char color_count;
    Color star_colors[3];
    float twinkle_x;
    float twinkle_speed;
    int padding_x;
    float screen_scroll;
} g_stars_data = {
    .screen_scroll = 0,
    .speeds_count = 3,
    .twinkle_speed = 2.5f,
    .twinkle_x = 0,
    .star_count = 200,
    .stars = {0},
    .color_count = 3,
    .star_colors = {
        (Color){.a = 255, .r = 255, .g = 255, .b = 255},
        (Color){.a = 255, .r = 255, .g = 255, .b = 220},
        (Color){.a = 255, .r = 220, .g = 240, .b = 255}},
    .star_speeds = {0},
    .padding_x = 0};

float g_transition_time = 0;     // The amount of time spent in the current transition
float g_transition_duration = 0; // The amount of time that will be spent in this transition
float g_transition_progress = 0; // How many percent of the transition duration has passed

Gamestate_e g_gamestate_previous = STATE_MAIN_MENU;
Gamestate_e g_gamestate_current = STATE_MAIN_MENU;

typedef struct
{
    Vector2 position;
    float time_lifetime;
    float time_alive;
    float size;
    Color color;
} Explosion_t;

struct
{
    float bounce_speed;
    float bounce_timer;
    float bounce_distance;
} g_main_menu_data = {
    .bounce_distance = 40,
    .bounce_timer = 0,
    .bounce_speed = 1.6f};

struct
{
    bool weapon_ready_alert;
} g_player_settings = {
    .weapon_ready_alert = false};

struct
{
    const int EXPLOSIONS_COUNT;

    Explosion_t explosions[50];
} g_explosions_data = {
    .EXPLOSIONS_COUNT = 50,
    .explosions = {0}};

typedef struct
{
    short damage;
    float radius;
    bool is_explosive;
    Explosion_t explosion;
} Projectile_Data_Player_t;

typedef struct
{
    int damage;
    int max_health;
    float radius;
    float speed;
    bool is_destroyable;
} Projectile_Data_Enemy_t;

typedef enum
{
    PROJECTILE_TYPE_PLAYER_NONE,
    PROJECTILE_TYPE_PLAYER_BURST,
    PROJECTILE_TYPE_PLAYER_CANNON,
    PROJECTILE_TYPE_PLAYER_AUTOCANNON,
    PROJECTILE_TYPE_PLAYER_TORPEDO,
    PROJECTILE_TYPE_PLAYER_COUNT
} Projectile_Type_Player_e;

typedef enum
{
    PROJECTILE_TYPE_ENEMY_NONE,
    PROJECTILE_TYPE_ENEMY_SHOOTER,
    PROJECTILE_TYPE_ENEMY_HEAVY_SHOOTER,
    PROJECTILE_TYPE_ENEMY_COUNT,
} Projectile_Type_Enemy_e;

typedef struct
{
    float time_wait;
    Vector2 velocity;
    Vector2 position;
    Projectile_Type_Player_e type;
    bool should_remove;
} Projectile_Player_t;

typedef struct
{
    int health;
    Vector2 velocity;
    Vector2 position;
    Projectile_Type_Enemy_e type;
    bool should_remove;
} Projectile_Enemy_t;

typedef struct
{
    unsigned char type;             // The weapon type
    float time_last_reload;         // Time elapsed since the ammo counter was last incremented
    unsigned char ammo_count;       // The ammo count will decrement each time the weapon is fired
    unsigned char ammo_count_max;   // The max amount of ammo this weapon can hold
    bool is_unlocked;               // If the gun can be used or not
    float time_ammo_reload;         // The time it takes to add 1 to the ammo counter
    float time_projectile_interval; // The interval between projectiles when firing
    int projectile_count;           // The number of projectiles to be spawned when firing
    unsigned char projectile_type;  // The type of the projectiles fired
    float start_velocity;           // The speed of the projectiles
    int spread;                     // The firing arc (if the value is 10, then the arc will be 10 in either direction, not 5)
    unsigned char firing_key;       // Which keyboard key must be pressed to fire the weapon
    int level_current;              // Upgrade level
} Weapon_t;

typedef enum
{
    WEAPON_TYPE_BURST,
    WEAPON_TYPE_CANNON,
    WEAPON_TYPE_AUTOCANNON,
    WEAPON_TYPE_TORPEDO,
    WEAPON_TYPE_COUNT
} Weapon_Type_e;

typedef enum
{
    ENEMY_TYPE_NONE,
    ENEMY_TYPE_LIGHT,
    ENEMY_TYPE_MEDIUM,
    ENEMY_TYPE_SHOOTER,
    ENEMY_TYPE_HEAVY_SHOOTER,
    ENEMY_TYPE_COUNT
} Enemy_Type_e;

typedef struct
{
    bool has_attempted_first_shot;
    Enemy_Type_e type;
    int current_health;
    Vector2 position;
    float time_last_fired;
    float time_healthbar_visible;
} Enemy_t;

typedef struct
{
    Color color;
    short height;
    short width;
    short speed;
    short max_health;
    unsigned char damage;
    bool can_shoot;
    int shoot_range;
    Projectile_Type_Enemy_e projectile_type;
    float time_firing_interval;
} Enemy_Data_t;

typedef struct
{
    unsigned char ammo_count_max;
    float time_ammo_reload;
    float time_projectile_interval;
    int projectile_count;
    float start_velocity;
    int spread;
    int damage;
} Weapon_Upgrade_Stats_t;

typedef struct
{
    float ammo_count_max;
    float time_ammo_reload;
    float time_projectile_interval;
    float projectile_count;
    float start_velocity;
    float spread;
    float damage;
} Weapon_Upgrade_Stat_Multipliers_t;

struct
{
    int upgrade_cost[WEAPON_TYPE_COUNT];
    float cost_scaling;
    Weapon_t weapons[WEAPON_TYPE_COUNT];
    Weapon_Upgrade_Stats_t base_stats[WEAPON_TYPE_COUNT];
    Weapon_Upgrade_Stat_Multipliers_t stat_increase_mutlipliers;
    int level_max;
    char names[WEAPON_TYPE_COUNT][25];

    float upgrade_transitions[WEAPON_TYPE_COUNT];
    float upgrade_transition_duration;

    float symbol_alert_timers[WEAPON_TYPE_COUNT];
    float symbol_alert_duration;

    Color symbol_color_default;
    Color symbol_color_alert;
    Color symbol_color_not_empty;
    Color symbol_color_full;

    unsigned char symbol_width;
    unsigned char symbol_height;
    short symbol_draw_area_y;
} g_weapons_data =
    {
        .upgrade_transition_duration = 0.15f,
        .upgrade_transitions = {0},
        .symbol_draw_area_y = 0,

        .symbol_alert_duration = 0.2f,
        .symbol_alert_timers = {0},
        .symbol_color_default = DARKBLUE,
        .symbol_color_not_empty = SKYBLUE,
        .symbol_color_full = GREEN,
        .symbol_color_alert = (Color){.a = 255, .r = SKYBLUE.r + 150, .g = SKYBLUE.g, .b = SKYBLUE.b},

        .symbol_width = 60,
        .symbol_height = 60,

        .names = {
            "Burst", "Cannon", "Autocannon", "Torpedo"},

        .upgrade_cost = {
            30,  // BURST
            50,  // CANNON
            75,  // AUTOCANNON
            125, // TORPEDO
        },

        .cost_scaling = 1.1f,

        .weapons = {{// BURST
                     .level_current = 0,
                     .time_last_reload = 0,
                     .ammo_count_max = 0,
                     .ammo_count = 0,
                     .is_unlocked = true,
                     .projectile_count = 0,
                     .type = WEAPON_TYPE_BURST,
                     .projectile_type = PROJECTILE_TYPE_PLAYER_BURST,
                     .start_velocity = 0,
                     .time_ammo_reload = 0,
                     .time_projectile_interval = 0,
                     .firing_key = KEY_ONE,
                     .spread = 0},

                    {// CANNON
                     .level_current = 0,
                     .time_last_reload = 0,
                     .ammo_count_max = 0,
                     .ammo_count = 0,
                     .is_unlocked = false,
                     .projectile_count = 0,
                     .type = WEAPON_TYPE_CANNON,
                     .projectile_type = PROJECTILE_TYPE_PLAYER_CANNON,
                     .start_velocity = 0,
                     .time_ammo_reload = 0,
                     .time_projectile_interval = 0,
                     .firing_key = KEY_TWO,
                     .spread = 0},

                    {// AUTOCANNON
                     .level_current = 0,
                     .time_last_reload = 0,
                     .ammo_count_max = 0,
                     .ammo_count = 0,
                     .is_unlocked = false,
                     .projectile_count = 0,
                     .type = WEAPON_TYPE_AUTOCANNON,
                     .projectile_type = PROJECTILE_TYPE_PLAYER_AUTOCANNON,
                     .start_velocity = 0,
                     .time_ammo_reload = 0,
                     .time_projectile_interval = 0,
                     .firing_key = KEY_THREE,
                     .spread = 3},

                    {// TORPEDO
                     .level_current = 0,
                     .time_last_reload = 0,
                     .ammo_count_max = 0,
                     .ammo_count = 0,
                     .is_unlocked = false,
                     .projectile_count = 0,
                     .type = WEAPON_TYPE_TORPEDO,
                     .projectile_type = PROJECTILE_TYPE_PLAYER_TORPEDO,
                     .start_velocity = 0,
                     .time_ammo_reload = 0,
                     .time_projectile_interval = 0,
                     .firing_key = KEY_FOUR,
                     .spread = 0}},

        .level_max = 10,

        .stat_increase_mutlipliers = {.ammo_count_max = 0.1f, .projectile_count = 0.05f, .damage = 0.12f, .spread = -0.03f, .time_ammo_reload = -0.05f},

        .base_stats = {{// BURST
                        .ammo_count_max = 4,
                        .time_projectile_interval = 0.05f,
                        .projectile_count = 6,
                        .start_velocity = 200,
                        .time_ammo_reload = 2,
                        .damage = 3,
                        .spread = 15},

                       {// CANNON
                        .ammo_count_max = 2,
                        .time_projectile_interval = 0,
                        .projectile_count = 1,
                        .start_velocity = 320,
                        .time_ammo_reload = 3.5f,
                        .damage = 30,
                        .spread = 0},

                       {// AUTOCANNON
                        .ammo_count_max = 2,
                        .time_projectile_interval = 0.04f,
                        .projectile_count = 3,
                        .start_velocity = 260,
                        .time_ammo_reload = 3,
                        .damage = 8,
                        .spread = 3},

                       {// TORPEDO
                        .ammo_count_max = 1,
                        .time_projectile_interval = 0,
                        .projectile_count = 1,
                        .start_velocity = 120,
                        .time_ammo_reload = 6,
                        .damage = 60,
                        .spread = 0}}};

struct
{
    const short OFFSET_Y; // The padding between the player and the bottom of the screen
    short player_current_health;
    short player_max_health;
    short planet_current_health;
    short planet_max_health;
    Color color;
    float hitbox_radius;
    float rotation;
    float rotation_speed;
    int money;
    Vector2 center;

    float transaction_timer;
    float transaction_timer_buffer;
    float transaction_duration;
    int transaction_money_remaining;
} g_player_data = {
    .OFFSET_Y = 200,
    .player_current_health = 20,
    .player_max_health = 20,
    .planet_current_health = 30,
    .planet_max_health = 30,
    .color = DARKBLUE,
    .hitbox_radius = 35,
    .rotation = 0,
    .rotation_speed = 30,
    .money = 0,
    .center = {.x = 0, .y = 0},

    .transaction_duration = 0.33f,
    .transaction_timer_buffer = 0,
    .transaction_timer = -1,
    .transaction_money_remaining = 0};

struct
{
    const int ENEMIES_COUNT;
    const float ENEMY_SPEED;

    int tallest_enemy_height;

    Enemy_t enemies[128];

    Enemy_Data_t enemy_database[ENEMY_TYPE_COUNT];

    float health_bar_visibility_duration;
    short healthbar_width;
} g_enemies_data = {
    .health_bar_visibility_duration = 2.5f,
    .healthbar_width = 60,

    .ENEMY_SPEED = 25,

    .ENEMIES_COUNT = 128,
    .enemies = {0},

    .enemy_database = {
        {// NONE
         .color = {0},
         .damage = 0,
         .max_health = 0,
         .height = 0,
         .width = 0,
         .can_shoot = false,
         .shoot_range = 0,
         .projectile_type = PROJECTILE_TYPE_ENEMY_NONE,
         .time_firing_interval = 0,
         .speed = 0},

        {// LIGHT
         .color = LIGHTGRAY,
         .damage = 4,
         .max_health = 25,
         .height = 40,
         .width = 25,
         .can_shoot = false,
         .shoot_range = 0,
         .projectile_type = PROJECTILE_TYPE_ENEMY_NONE,
         .time_firing_interval = 0,
         .speed = 25},

        {// MEDIUM
         .color = GRAY,
         .damage = 10,
         .max_health = 60,
         .height = 60,
         .width = 35,
         .shoot_range = 0,
         .can_shoot = false,
         .projectile_type = PROJECTILE_TYPE_ENEMY_NONE,
         .time_firing_interval = 0,
         .speed = 25},

        {// SHOOTER
         .color = RED,
         .damage = 4,
         .max_health = 30,
         .height = 40,
         .width = 35,
         .can_shoot = true,
         .shoot_range = 250,
         .projectile_type = PROJECTILE_TYPE_ENEMY_SHOOTER,
         .time_firing_interval = 999,
         .speed = 25},

        {// HEAVY_SHOOTER
         .color = DARKPURPLE,
         .damage = 5,
         .max_health = 45,
         .height = 50,
         .width = 40,
         .can_shoot = true,
         .shoot_range = 9999,
         .projectile_type = PROJECTILE_TYPE_ENEMY_HEAVY_SHOOTER,
         .time_firing_interval = 999,
         .speed = 25}}};

struct
{
    int player_projectile_count;

    Projectile_Player_t player_projectiles[512];
    Projectile_Data_Player_t player_projectile_database[PROJECTILE_TYPE_PLAYER_COUNT];

    int enemy_projectile_count;

    Projectile_Enemy_t enemy_projectiles[128];
    Projectile_Data_Enemy_t enemy_projectile_database[PROJECTILE_TYPE_PLAYER_COUNT];
} g_projectiles_data = {
    .player_projectiles = {0},
    .player_projectile_count = 512,
    .player_projectile_database =
        {{// NONE
          .damage = 0,
          .radius = 0,
          .is_explosive = false,
          .explosion = {
              .position = (Vector2){.x = -1, .y = -1},
              .size = 30,
              .time_lifetime = 0.15f,
              .time_alive = 0,
              .color = (Color){.a = 255, .b = 0, .g = 200, .r = 255}}},

         {// BURST
          .damage = 4,
          .radius = 3,
          .is_explosive = false,
          .explosion = {.position = (Vector2){.x = -1, .y = -1}, .size = 15, .time_lifetime = 0.10f, .time_alive = 0, .color = (Color){.a = 255, .b = 175, .g = 255, .r = 255}}},

         {// CANNON
          .damage = 500,
          .radius = 8,
          .is_explosive = false,
          .explosion = {.position = (Vector2){.x = -1, .y = -1}, .size = 20, .time_lifetime = 0.10f, .time_alive = 0, .color = (Color){.a = 255, .b = 175, .g = 255, .r = 255}}},

         {// AUTOCANNON
          .damage = 4,
          .radius = 5,
          .is_explosive = true,
          .explosion = {.position = (Vector2){.x = -1, .y = -1}, .size = 50, .time_lifetime = 0.15f, .time_alive = 0, .color = (Color){.a = 255, .b = 0, .g = 160, .r = 255}}},

         {// TORPEDO
          .damage = 200,
          .radius = 20,
          .is_explosive = true,
          .explosion = {.position = (Vector2){.x = -1, .y = -1}, .size = 150, .time_lifetime = 0.3f, .time_alive = 0, .color = (Color){.a = 255, .b = 0, .g = 160, .r = 255}}}},

    .enemy_projectile_count = 128,
    .enemy_projectiles = {0},
    .enemy_projectile_database = {{// NONE
                                   .is_destroyable = false,
                                   .max_health = 0,
                                   .damage = 0,
                                   .radius = 0,
                                   .speed = 0},

                                  {// SHOOTER
                                  .is_destroyable = false,
                                   .max_health = 0,
                                   .damage = 5,
                                   .radius = 4,
                                   .speed = 60},

                                  {// HEAVY SHOOTER
                                   .is_destroyable = true,
                                   .max_health = 15,
                                   .damage = 8,
                                   .radius = 15,
                                   .speed = 50}}};

typedef struct
{
    float spawn_credits_build_rate;
    float spawn_credits_target;

    float time_next_wave_min;
    float time_next_wave_max;

    float enemy_spawn_chances[ENEMY_TYPE_COUNT];
    float enemy_spawn_chance_factor[ENEMY_TYPE_COUNT];
} Level_Data_t;

struct
{
    float spawn_y;
    float spawn_y_min;

    float spawn_credits;
    float total_spawn_credits;

    float time_next_wave;

    float enemy_spawn_costs[ENEMY_TYPE_COUNT];
    float enemy_spawn_chances[ENEMY_TYPE_COUNT];

    short current_level;

    short next_level;
    short max_level;
} g_enemy_spawn_director = {
    .spawn_y = 0,
    .spawn_y_min = -100,

    .spawn_credits = 0,
    .total_spawn_credits = 0,

    .time_next_wave = 0,

    .enemy_spawn_chances = {0},
    .enemy_spawn_costs = {999999, 10, 20, 20, 30},

    .next_level = 0, // TODO - Add win condition for when next_level passes max_level
    .max_level = 29,
    .current_level = 0};

struct
{
    const int LEVEL_COUNT;
    Level_Data_t levels[30];
} g_levels_data = {
    // clang-format off
    .LEVEL_COUNT = 30,
    .levels = {
    {   // Level 1
        .spawn_credits_build_rate = 3,
        .spawn_credits_target = 150,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 1, 0, 0, 0},
        .enemy_spawn_chance_factor = { 0, 1, 0, 0, 0},
    },
    {   // Level 2
        .spawn_credits_build_rate = 3,
        .spawn_credits_target = 250,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, (2 / 3.0f), (1 / 3.0f), 0, 0},
        .enemy_spawn_chance_factor = { 0, 0.5f, 0.5f, 0},
    },
    {   // Level 3
        .spawn_credits_build_rate = 3.5f,
        .spawn_credits_target = 300,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, (2 / 3.0f), (1 / 3.0f), 0, 0},
        .enemy_spawn_chance_factor = { 0, 0.5f, 0.5f, 0, 0},
    },
    {   // Level 4
        .spawn_credits_build_rate = 4,
        .spawn_credits_target = 320,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, (1 / 3.0f), (2 / 3.0f), 0, 0},
        .enemy_spawn_chance_factor = { 0, 0.5f, 0.5f, 0, 0},
    },
    {   // Level 5
        .spawn_credits_build_rate = 4.5,
        .spawn_credits_target = 360,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, (1 / 4.0f), (2 / 4.0f), (1 / 4.0f), 0},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.75f, 0.05f, 0},
    },
    {   // Level 6
        .spawn_credits_build_rate = 4.5,
        .spawn_credits_target = 300,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, (1 / 5.0f), (1 / 5.0f), (1 / 5.0f), (2 / 5.0f)},
        .enemy_spawn_chance_factor = { 0, 0.25f, 0.25f, 0.25f, 0.5f},
    },
    // TODO
    // Balance the rest of the levels
    {   // Level 7
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 8
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 9
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 10
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 11
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 12
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 13
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 14
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 15
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 16
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 17
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 18
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 19
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 20
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 21
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 22
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 23
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 24
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 25
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 26
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 27
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 28
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 29
        .spawn_credits_build_rate = 2,
        .spawn_credits_target = 80,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, 0.80f, 0.20f},
        .enemy_spawn_chance_factor = { 0, 0.75f, 0.25f},
    },
    {   // Level 30
        .spawn_credits_build_rate = 15,
        .spawn_credits_target = 400,

        .time_next_wave_min = 10.0f,
        .time_next_wave_max = 10.0f,

        .enemy_spawn_chances = { 0, (1.0f / (ENEMY_TYPE_COUNT - 1)), (1.0f / (ENEMY_TYPE_COUNT - 1)), (1.0f / (ENEMY_TYPE_COUNT - 1)), (1.0f / (ENEMY_TYPE_COUNT - 1))},
        .enemy_spawn_chance_factor = { 0, 1, 1, 1, 1},
    }}};
// clang-format on

// Returns a random number between 0 and 1
float rand_float()
{
    return rand() / (float)RAND_MAX;
}

float rand_range_float(float inclusive_min, float inclusive_max)
{
    return rand_float() * (inclusive_max - inclusive_min) + inclusive_min;
}

int rand_range_int(int inclusive_min, int exclusive_max)
{
    return (rand() % (exclusive_max - inclusive_min)) + inclusive_min;
}

void shuffle_array_int(int array[], int element_count)
{
    for (int i = element_count - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);

        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void explosion_spawn(const Vector2 POSITION, const float LIFETIME, const float SIZE, Color color)
{
    Explosion_t new_explosion = {0};
    new_explosion.position = POSITION;
    new_explosion.size = SIZE;

    new_explosion.color = color;

    new_explosion.time_lifetime = LIFETIME;
    new_explosion.time_alive = 0;

    for (int i = 0; i < g_explosions_data.EXPLOSIONS_COUNT; i++)
    {
        if (g_explosions_data.explosions[i].time_alive < g_explosions_data.explosions[i].time_lifetime)
        {
            continue;
        }

        g_explosions_data.explosions[i] = new_explosion;
        break;
    }
}

void explosions_update()
{
    for (int i = 0; i < g_explosions_data.EXPLOSIONS_COUNT; i++)
    {
        if (g_explosions_data.explosions[i].time_alive >= g_explosions_data.explosions[i].time_lifetime)
        {
            continue;
        }

        g_explosions_data.explosions[i].time_alive += g_frame_time;
    }
}

void money_remove(int money)
{
    g_player_data.money -= money;
    g_player_data.transaction_money_remaining += money;

    g_player_data.transaction_timer_buffer = 0;

    g_player_data.transaction_timer = g_player_data.transaction_duration;
}

void money_add(int money)
{
    g_player_data.money += money;
    g_player_data.transaction_money_remaining -= money;

    g_player_data.transaction_timer_buffer = 0;

    g_player_data.transaction_timer = g_player_data.transaction_duration;
}

void money_update()
{
    if (g_player_data.transaction_timer < 0)
    {
        g_player_data.transaction_money_remaining = 0;
        return;
    }

    int money_buffer = 0;
    if (g_player_data.transaction_money_remaining > 0)
    {
        money_buffer = (int)floor((g_player_data.transaction_money_remaining * (g_frame_time + g_player_data.transaction_timer_buffer) / g_player_data.transaction_timer));
    }
    else
    {
        money_buffer = (int)ceil((g_player_data.transaction_money_remaining * (g_frame_time + g_player_data.transaction_timer_buffer) / g_player_data.transaction_timer));
    }

    if (money_buffer)
    {
        g_player_data.transaction_money_remaining -= money_buffer;
        g_player_data.transaction_timer_buffer = 0;
    }
    else
    {
        g_player_data.transaction_timer_buffer += g_frame_time;
    }

    g_player_data.transaction_timer -= g_frame_time;
}

void weapons_init()
{
    for (size_t i = 0; i < WEAPON_TYPE_COUNT; i++)
    {
        Weapon_t *weapon = &g_weapons_data.weapons[i];
        Weapon_Upgrade_Stats_t *base_stats = &g_weapons_data.base_stats[i];
        Weapon_Upgrade_Stat_Multipliers_t *stat_multipliers = &g_weapons_data.stat_increase_mutlipliers;

        weapon->ammo_count_max = base_stats->ammo_count_max * (1.0f + stat_multipliers->ammo_count_max * weapon->level_current);

        weapon->time_ammo_reload = base_stats->time_ammo_reload * (1.0f + stat_multipliers->time_ammo_reload * weapon->level_current);

        weapon->time_projectile_interval = base_stats->time_projectile_interval * (1.0f + stat_multipliers->time_projectile_interval * weapon->level_current);

        weapon->projectile_count = base_stats->projectile_count * (1.0f + stat_multipliers->projectile_count * weapon->level_current);

        weapon->start_velocity = base_stats->start_velocity * (1.0f + stat_multipliers->start_velocity * weapon->level_current);

        weapon->spread = base_stats->spread * (1.0f + stat_multipliers->spread * weapon->level_current);

        weapon->time_last_reload = 0;

        weapon->ammo_count = 0;

        g_weapons_data.symbol_alert_timers[i] = 999;

        g_projectiles_data.player_projectile_database[weapon->projectile_type].damage = base_stats->damage * (1.0f + stat_multipliers->damage * weapon->level_current);
    }
}

void background_init()
{
    g_stars_data.padding_x = g_window.width / 2;

    for (int i = 0; i < g_stars_data.star_count; i++)
    {
        g_stars_data.stars[i] = (Star_t){
            .color_index = rand() % g_stars_data.color_count,
            .position = (Vector2){.x = rand_range_int(-g_stars_data.padding_x - 5, g_window.width + g_stars_data.padding_x + 5), .y = rand_range_int(-5, g_window.height + 5)},
            .radius = rand_range_int(1, 3),
            .twinkle_offset = rand_float() * (PI * 2),
            .speed_index = rand() % g_stars_data.speeds_count};
    }

    Vector2 base_speed = {.x = cos(135 * DEG2RAD), .y = sin(135 * DEG2RAD)};
    base_speed = Vector2Scale(base_speed, 30);

    for (int i = 0; i < g_stars_data.speeds_count; i++)
    {
        g_stars_data.star_speeds[i] = Vector2Scale(base_speed, 1 + 0.2f * i);
    }
}

void player_init()
{
    g_player_data.color = BLUE;
    g_player_data.planet_current_health = g_player_data.planet_max_health;
    g_player_data.player_current_health = g_player_data.player_max_health;
    g_player_data.center = (Vector2){.x = g_window.width / 2, .y = g_window.height - g_player_data.OFFSET_Y};
}

void projectiles_init()
{
    for (int i = 0; i < g_projectiles_data.player_projectile_count; i++)
    {
        g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
    }

    for (int i = 0; i < g_projectiles_data.enemy_projectile_count; i++)
    {
        g_projectiles_data.enemy_projectiles[i].type = PROJECTILE_TYPE_ENEMY_NONE;
    }
}

void explosions_init()
{
    for (int i = 0; i < g_explosions_data.EXPLOSIONS_COUNT; i++)
    {
        g_explosions_data.explosions[i].time_lifetime = -1;
        g_explosions_data.explosions[i].time_alive = 1;
    }
}

void enemy_take_damage(int enemy_i, int damage)
{
    Enemy_t *enemy = &g_enemies_data.enemies[enemy_i];

    enemy->current_health -= damage;
    enemy->time_healthbar_visible = 0;
}

float enemies_find_tallest_height()
{
    float height = 0;
    for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
    {
        if (height > g_enemies_data.enemy_database[i].height)
        {
            continue;
        }

        height = g_enemies_data.enemy_database[i].height;
    }
    return height;
}

void enemies_init()
{
    for (int i = 0; i < g_enemies_data.ENEMIES_COUNT; i++)
    {
        g_enemies_data.enemies[i].type = ENEMY_TYPE_NONE;
        g_enemies_data.enemies[i].current_health = 0;
        g_enemies_data.enemies[i].time_healthbar_visible = 0;
    }

    g_enemies_data.tallest_enemy_height = enemies_find_tallest_height();
}

void enemy_spawn_director_init_level(int level)
{
    if (level > g_enemy_spawn_director.max_level)
    {
        level = g_enemy_spawn_director.max_level;
    }
    g_enemy_spawn_director.current_level = level;

    g_gamestate_current = STATE_LEVEL;

    for (int i = 0; i < ENEMY_TYPE_COUNT - 1; i++)
    {
        g_enemy_spawn_director.enemy_spawn_chances[i] = g_levels_data.levels[g_enemy_spawn_director.current_level].enemy_spawn_chances[i];
    }

    g_enemy_spawn_director.spawn_y = g_enemy_spawn_director.spawn_y_min;
    g_enemy_spawn_director.time_next_wave = 0;
    g_enemy_spawn_director.spawn_credits = g_levels_data.levels[level].time_next_wave_min * g_levels_data.levels[level].spawn_credits_build_rate;
    g_enemy_spawn_director.total_spawn_credits = 0;

    for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
    {
        g_enemy_spawn_director.enemy_spawn_chances[i] = g_levels_data.levels[g_enemy_spawn_director.current_level].enemy_spawn_chances[i];
    }

    for (int i = 0; i < g_projectiles_data.player_projectile_count; i++)
    {
        g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
    }

    player_init();
    projectiles_init();
    enemies_init();
    weapons_init();
}

Vector2 enemy_get_center(Enemy_t enemy)
{
    float width = g_enemies_data.enemy_database[enemy.type].width;
    float height = g_enemies_data.enemy_database[enemy.type].height;

    Vector2 center = Vector2Subtract(Vector2Add((Vector2){.x = width, .y = height}, enemy.position), enemy.position); // Points towards the center of the enemy
    center = Vector2Normalize(center);

    float distance = hypotf(width, height) / 2; // The distance between the position of the enemy and its center

    center = Vector2Scale(center, distance);     // Scale the vector so that the vector end point is in the center of the enemy
    center = Vector2Add(center, enemy.position); // Add the vector to get absolute position of enemy center

    return center;
}

void enemies_update()
{
    for (unsigned char i = 0; i < g_enemies_data.ENEMIES_COUNT; i++)
    {
        Enemy_t *current_enemy = &g_enemies_data.enemies[i];

        // DrawCircleV(enemy_get_center(g_enemies_data.enemies[i]), 8, PURPLE);

        if (current_enemy->type == ENEMY_TYPE_NONE)
        {
            continue;
        }

        if (current_enemy->current_health <= 0)
        {
            explosion_spawn(enemy_get_center(*current_enemy), 0.30f, 60, ORANGE);

            money_add((int)floor(g_enemy_spawn_director.enemy_spawn_costs[current_enemy->type]));
            current_enemy->type = ENEMY_TYPE_NONE;
            current_enemy->current_health = 1;
        }

        current_enemy->time_healthbar_visible += g_frame_time;
        current_enemy->position.y += g_enemies_data.enemy_database[current_enemy->type].speed * g_frame_time;

        if (current_enemy->position.y > 0 && g_enemies_data.enemy_database[current_enemy->type].can_shoot)
        {
            current_enemy->time_last_fired += g_frame_time;

            Vector2 enemy_to_player = Vector2Subtract(g_player_data.center, enemy_get_center(*current_enemy));
            if (Vector2Length(enemy_to_player) < g_enemies_data.enemy_database[current_enemy->type].shoot_range && current_enemy->time_last_fired > g_enemies_data.enemy_database[current_enemy->type].time_firing_interval && (current_enemy->position.y < g_player_data.center.y || current_enemy->has_attempted_first_shot))
            {
                if(!current_enemy->has_attempted_first_shot)
                {
                    current_enemy->has_attempted_first_shot = true;
                    current_enemy->time_last_fired = g_enemies_data.enemy_database[current_enemy->type].time_firing_interval - 3 * rand_float();
                    continue;
                }

                for (int j = 0; j < g_projectiles_data.enemy_projectile_count; j++)
                {
                    if (g_projectiles_data.enemy_projectiles[j].type != PROJECTILE_TYPE_ENEMY_NONE)
                    {
                        continue;
                    }

                    Projectile_Type_Enemy_e new_type = g_enemies_data.enemy_database[current_enemy->type].projectile_type;
                    Vector2 new_position = enemy_get_center(*current_enemy);
                    Projectile_Enemy_t new_projectile = {
                        .position = new_position,
                        .should_remove = false,
                        .health = g_projectiles_data.enemy_projectile_database[new_type].max_health,
                        .type = new_type,
                        .velocity = Vector2Scale(Vector2Normalize(Vector2Subtract(g_player_data.center, new_position)), g_projectiles_data.enemy_projectile_database[g_enemies_data.enemy_database[g_enemies_data.enemies[i].type].projectile_type].speed)};

                    g_projectiles_data.enemy_projectiles[j] = new_projectile;
                    current_enemy->time_last_fired -= g_enemies_data.enemy_database[current_enemy->type].time_firing_interval;

                    break;
                }
            }
        }

        if (current_enemy->position.y > g_weapons_data.symbol_draw_area_y)
        {
            g_player_data.planet_current_health -= g_enemies_data.enemy_database[current_enemy->type].damage;
            current_enemy->type = ENEMY_TYPE_NONE;
        }
    }
}

void enemies_kill_all()
{
    for (int i = 0; i < g_enemies_data.ENEMIES_COUNT; i++)
    {
        g_enemies_data.enemies[i].type = ENEMY_TYPE_NONE;
    }
}

void enemies_end_level()
{
    enemies_kill_all();
    g_enemy_spawn_director.spawn_credits = -1;
    g_enemy_spawn_director.total_spawn_credits = 999999;
}

bool enemies_all_dead()
{
    for (int i = 0; i < g_enemies_data.ENEMIES_COUNT; i++)
    {
        if (g_enemies_data.enemies[i].type != ENEMY_TYPE_NONE)
        {
            return false;
        }
    }

    return true;
}

void enemies_spawn_wave()
{
    // TODO
    // Don't store enemy_type_none in affordable_enemies

    // clang-format off
    enum { SPAWN_ATTEMPTS = 3 };
    enum { ENEMIES_PER_INTERVAL = 5 };
    const float SPAWN_INTERVAL_Y = g_enemies_data.ENEMY_SPEED * g_enemy_spawn_director.time_next_wave;
    enum { ENEMY_SPAWN_PADDING = 10 };
    // clang-format on

    struct
    {
        float spawn_chance;
        Enemy_Type_e enemy_type;
    } affordable_enemies[ENEMY_TYPE_COUNT] = {0}; // A list of all enemies that can be afforded

    int afford_count = 0;
    for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
    {
        if (g_enemy_spawn_director.enemy_spawn_costs[i] > g_enemy_spawn_director.spawn_credits)
        {
            continue;
        }

        affordable_enemies[afford_count].enemy_type = i;
        affordable_enemies[afford_count].spawn_chance = g_levels_data.levels[g_enemy_spawn_director.current_level].enemy_spawn_chances[i];

        afford_count++;
    }

    while (afford_count)
    {
        {
            // Balance affordable enemies
            float total_chance = 0;
            for (int i = 0; i < afford_count; i++)
            {
                total_chance += affordable_enemies[i].spawn_chance;
            }

            float difference = 1.0f / total_chance;

            for (int i = 0; i < afford_count; i++)
            {
                affordable_enemies[i].spawn_chance *= difference;
            }
        }

        // TODO
        // "spawn_weight" is not descriptive enough
        float spawn_weight = rand() / (float)RAND_MAX;

        Enemy_Type_e new_enemy_type = ENEMY_TYPE_NONE;

        float running_total = 0;

        for (int i = 0; i < afford_count; i++)
        {
            if (spawn_weight < running_total || spawn_weight > running_total + affordable_enemies[i].spawn_chance)
            {
                running_total += affordable_enemies[i].spawn_chance;
                continue;
            }

            // running_total += affordable_enemies[i].spawn_chance;
            new_enemy_type = affordable_enemies[i].enemy_type;

            break;
        }

        {
            g_enemy_spawn_director.enemy_spawn_chances[new_enemy_type] *= g_levels_data.levels[g_enemy_spawn_director.current_level].enemy_spawn_chance_factor[new_enemy_type];

            // Balance absolute spawn chances
            float total_chance = 0;
            for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
            {
                total_chance += g_enemy_spawn_director.enemy_spawn_chances[i];
            }

            float difference = 1.0f / total_chance;

            for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
            {
                g_enemy_spawn_director.enemy_spawn_chances[i] *= difference;
            }
        }

        // Find a free enemy index:
        for (int i = 0; i < g_enemies_data.ENEMIES_COUNT; i++)
        {
            // If at last index, return: else continue
            if (g_enemies_data.enemies[i].type != ENEMY_TYPE_NONE)
            {
                if (i == g_enemies_data.ENEMIES_COUNT - 1)
                {
                    return;
                }

                continue;
            }

            float new_x = rand_range_int(0, (g_window.width - g_enemies_data.enemy_database[new_enemy_type].width));
            float new_y = -rand_range_int((-1) * g_enemy_spawn_director.spawn_y, (-1) * g_enemy_spawn_director.spawn_y + SPAWN_INTERVAL_Y);

            // Move new enemy if it collides with any pre-existing ones
            for (int i = 0; i < 10; i++)
            {
                bool is_colliding = false;

                Rectangle current_enemy_hitbox;
                Rectangle new_enemy_hitbox;

                for (int j = 0; j < g_enemies_data.ENEMIES_COUNT; j++)
                {
                    current_enemy_hitbox = (Rectangle){
                        .x = g_enemies_data.enemies[j].position.x,
                        .y = g_enemies_data.enemies[j].position.y,
                        .width = g_enemies_data.enemy_database[g_enemies_data.enemies[j].type].width,
                        .height = g_enemies_data.enemy_database[g_enemies_data.enemies[j].type].height};

                    new_enemy_hitbox = (Rectangle){
                        .x = new_x,
                        .y = new_y,
                        .width = g_enemies_data.enemy_database[new_enemy_type].width,
                        .height = g_enemies_data.enemy_database[new_enemy_type].height};

                    if (CheckCollisionRecs(current_enemy_hitbox, new_enemy_hitbox))
                    {
                        is_colliding = true;
                        break;
                    }
                }

                if (!is_colliding)
                {
                    break;
                }

                new_x = rand_range_int(0, (g_window.width - g_enemies_data.enemy_database[new_enemy_type].width));
                new_y = -rand_range_int((-1) * g_enemy_spawn_director.spawn_y, (-1) * g_enemy_spawn_director.spawn_y + SPAWN_INTERVAL_Y);
            }

            Enemy_t new_enemy = {
                .type = new_enemy_type,
                .current_health = g_enemies_data.enemy_database[new_enemy_type].max_health,
                .time_last_fired = g_enemies_data.enemy_database[new_enemy_type].time_firing_interval,
                .has_attempted_first_shot = false,
                .position = (Vector2){.x = new_x,
                                      .y = new_y}};

            g_enemies_data.enemies[i] = new_enemy;
            g_enemy_spawn_director.spawn_credits -= g_enemy_spawn_director.enemy_spawn_costs[new_enemy_type];

            break;
        }

        for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
        {
            affordable_enemies[i].enemy_type = ENEMY_TYPE_NONE;
            affordable_enemies[i].spawn_chance = 0;
        }

        afford_count = 0;
        for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
        {
            if (g_enemy_spawn_director.enemy_spawn_costs[i] > g_enemy_spawn_director.spawn_credits)
            {
                continue;
            }

            affordable_enemies[afford_count].enemy_type = i;
            affordable_enemies[afford_count].spawn_chance = g_enemy_spawn_director.enemy_spawn_chances[i];

            afford_count++;
        }
    }

    g_enemy_spawn_director.spawn_y -= SPAWN_INTERVAL_Y;
}

void enemies_update_spawn_conditions()
{
    if (IsKeyPressed(KEY_O))
    {
        enemies_kill_all();
    }

    if (IsKeyPressed(KEY_P))
    {
        enemies_end_level();
    }

    if (g_enemy_spawn_director.total_spawn_credits > g_levels_data.levels[g_enemy_spawn_director.current_level].spawn_credits_target)
    {
        if (g_enemy_spawn_director.spawn_credits < 0)
        {
            if (enemies_all_dead())
            {
                g_gamestate_previous = STATE_LEVEL;
                g_gamestate_current = STATE_UPGRADE;
                g_transition_time = 0;
                g_transition_duration = 4;
                g_enemy_spawn_director.next_level++;
                if (g_enemy_spawn_director.next_level > g_enemy_spawn_director.max_level)
                {
                    g_enemy_spawn_director.next_level = g_enemy_spawn_director.max_level;
                }
                projectiles_init();
            }
            return;
        }

        g_enemy_spawn_director.time_next_wave += rand_range_float(g_levels_data.levels[g_enemy_spawn_director.current_level].time_next_wave_min, g_levels_data.levels[g_enemy_spawn_director.current_level].time_next_wave_max);
        enemies_spawn_wave();
        g_enemy_spawn_director.spawn_credits = -1;
        return;
    }

    (g_enemy_spawn_director.spawn_y) += g_enemies_data.ENEMY_SPEED * g_frame_time;
    if (g_enemy_spawn_director.spawn_y > g_enemy_spawn_director.spawn_y_min)
    {
        g_enemy_spawn_director.spawn_y = g_enemy_spawn_director.spawn_y_min;
    }

    g_enemy_spawn_director.time_next_wave -= g_frame_time;

    g_enemy_spawn_director.spawn_credits += g_levels_data.levels[g_enemy_spawn_director.current_level].spawn_credits_build_rate * g_frame_time;
    g_enemy_spawn_director.total_spawn_credits += g_levels_data.levels[g_enemy_spawn_director.current_level].spawn_credits_build_rate * g_frame_time;

    if (g_enemy_spawn_director.time_next_wave > 0)
    {
        return;
    }

    g_enemy_spawn_director.time_next_wave += rand_range_float(g_levels_data.levels[g_enemy_spawn_director.current_level].time_next_wave_min, g_levels_data.levels[g_enemy_spawn_director.current_level].time_next_wave_max);
    enemies_spawn_wave();
}

void enemies_draw()
{
    for (int i = 0; i < g_enemies_data.ENEMIES_COUNT; i++)
    {
        Enemy_t *current_enemy = &g_enemies_data.enemies[i];
        if (g_enemies_data.enemies[i].type == ENEMY_TYPE_NONE)
        {
            continue;
        }

        // Enemy
        DrawRectangle(
            current_enemy->position.x,
            current_enemy->position.y,
            g_enemies_data.enemy_database[current_enemy->type].width,
            g_enemies_data.enemy_database[current_enemy->type].height,
            g_enemies_data.enemy_database[current_enemy->type].color);

        if (current_enemy->time_healthbar_visible < g_enemies_data.health_bar_visibility_duration)
        {
            // Healthbar base
            DrawRectangle(
                enemy_get_center(*current_enemy).x - g_enemies_data.healthbar_width / 2,
                current_enemy->position.y - 10,
                g_enemies_data.healthbar_width,
                8,
                RED);

            // Healthbar fill
            DrawRectangle(
                enemy_get_center(*current_enemy).x - g_enemies_data.healthbar_width / 2,
                current_enemy->position.y - 10,
                Clamp((float)g_enemies_data.healthbar_width * (current_enemy->current_health / (float)g_enemies_data.enemy_database[current_enemy->type].max_health), 0, g_enemies_data.healthbar_width),
                8,
                GREEN);
        }
    }
}

void explosion_spawn_expl(const Explosion_t EXPLOSION)
{
    for (int i = 0; i < g_explosions_data.EXPLOSIONS_COUNT; i++)
    {
        if (g_explosions_data.explosions[i].time_alive < g_explosions_data.explosions[i].time_lifetime)
        {
            continue;
        }

        g_explosions_data.explosions[i] = EXPLOSION;
        break;
    }
}

bool create_button_text(bool disabled, int center_x, int center_y, int width, int height, Color button_color, bool change_on_hover, Color hover_color, char *text, int font_size, Color text_color, int spacing)
{
    int origin_x = center_x - width / 2;
    int origin_y = center_y - height / 2;

    if (change_on_hover && CheckCollisionPointRec(g_mouse_position, (Rectangle){origin_x, origin_y, width, height}))
    {
        button_color = hover_color;
    }

    DrawRectangle(origin_x, origin_y, width, height, button_color);

    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, spacing);
    DrawTextEx(GetFontDefault(), text, (Vector2){.x = center_x - (text_size.x / 2), .y = center_y - (text_size.y / 2)}, font_size, spacing, text_color);

    if (disabled)
    {
        return false;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }
    return CheckCollisionPointRec(g_mouse_position, (Rectangle){.x = origin_x, .y = origin_y, .width = width, .height = height});
}

bool create_button_text_border(bool disabled, int center_x, int center_y, int width, int height, Color button_color, bool change_on_hover, Color hover_color, const char *text, int font_size, Color text_color, int spacing, int border_thickness, Color border_color)
{
    int origin_x = center_x - width / 2;
    int origin_y = center_y - height / 2;

    if (change_on_hover && CheckCollisionPointRec(g_mouse_position, (Rectangle){origin_x, origin_y, width, height}))
    {
        button_color = hover_color;
    }

    DrawRectangle(origin_x, origin_y, width, height, button_color);
    DrawRectangleLinesEx(
        (Rectangle){.x = origin_x, .y = origin_y, .width = width, .height = height},
        border_thickness,
        border_color);

    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, spacing);
    DrawTextEx(GetFontDefault(), text, (Vector2){.x = center_x - (text_size.x / 2), .y = center_y - (text_size.y / 2)}, font_size, spacing, text_color);

    if (disabled)
    {
        return false;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }
    return CheckCollisionPointRec(g_mouse_position, (Rectangle){.x = origin_x, .y = origin_y, .width = width, .height = height});
}

bool create_button_border(bool disabled, int center_x, int center_y, int width, int height, Color button_color, bool change_on_hover, Color hover_color, int border_thickness, Color border_color)
{
    int origin_x = center_x - width / 2;
    int origin_y = center_y - height / 2;

    if (CheckCollisionPointRec(g_mouse_position, (Rectangle){.x = origin_x, origin_y, width, height}) && change_on_hover)
    {
        button_color = hover_color;
    }

    DrawRectangle(origin_x, origin_y, width, height, button_color);
    DrawRectangleLinesEx(
        (Rectangle){.x = origin_x, .y = origin_y, .width = width, .height = height},
        border_thickness,
        border_color);

    if (disabled)
    {
        return false;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }
    return CheckCollisionPointRec(g_mouse_position, (Rectangle){.x = origin_x, .y = origin_y, .width = width, .height = height});
}

void projectile_check_collision(Projectile_Player_t *projectile)
{
    // TODO
    // Enemies outside the screen should not be able to be damaged by projectiles, only their explosions

    for (int enemy_index = 0; enemy_index < g_enemies_data.ENEMIES_COUNT; enemy_index++)
    {
        Enemy_t *current_enemy = &g_enemies_data.enemies[enemy_index];

        if (current_enemy->type == ENEMY_TYPE_NONE)
        {
            continue;
        }

        if (current_enemy->current_health <= 0)
        {
            continue;
        }

        if (current_enemy->position.y <= -g_enemies_data.enemy_database[current_enemy->type].height)
        {
            continue;
        }

        // Check collision
        if (CheckCollisionCircleRec(
                projectile->position,
                g_projectiles_data.player_projectile_database[projectile->type].radius,
                (Rectangle){
                    .height = g_enemies_data.enemy_database[current_enemy->type].height,
                    .width = g_enemies_data.enemy_database[current_enemy->type].width,
                    .x = current_enemy->position.x,
                    .y = current_enemy->position.y}))
        {
            Explosion_t new_explosion = g_projectiles_data.player_projectile_database[projectile->type].explosion;
            new_explosion.position = projectile->position;

            explosion_spawn_expl(new_explosion);

            // If the projectile is explosive
            if (g_projectiles_data.player_projectile_database[projectile->type].is_explosive)
            {
                // Damage all enemies inside the explosion
                for (int explosion_check_enemy_index = 0; explosion_check_enemy_index < g_enemies_data.ENEMIES_COUNT; explosion_check_enemy_index++)
                {
                    if (CheckCollisionCircleRec(
                            projectile->position,
                            g_projectiles_data.player_projectile_database[projectile->type].explosion.size,
                            (Rectangle){
                                .height = g_enemies_data.enemy_database[g_enemies_data.enemies[explosion_check_enemy_index].type].height,
                                .width = g_enemies_data.enemy_database[g_enemies_data.enemies[explosion_check_enemy_index].type].width,
                                .x = g_enemies_data.enemies[explosion_check_enemy_index].position.x,
                                .y = g_enemies_data.enemies[explosion_check_enemy_index].position.y}))
                    {
                        Vector2 enemy_center = enemy_get_center(g_enemies_data.enemies[explosion_check_enemy_index]);

                        float distance = Vector2Distance(enemy_center, projectile->position); // The distance between the enemy center and the projectile center

                        float factor = 1 - (distance / g_projectiles_data.player_projectile_database[projectile->type].explosion.size); // How close to the origin of the explosion radius the enemy is
                        factor = Clamp(factor, 0, 1);

                        enemy_take_damage(explosion_check_enemy_index, g_projectiles_data.player_projectile_database[projectile->type].damage * (factor));
                    }
                }
            }
            // If the projectile is not explosive
            else
            {
                enemy_take_damage(enemy_index, g_projectiles_data.player_projectile_database[projectile->type].damage);
            }

            projectile->should_remove = true;
            break;
        }
    }
}

void projectile_player_spawn(const int WEAPON_INDEX, const float TIME_WAIT)
{
    Projectile_Player_t new_projectile = {0};

    new_projectile.time_wait = TIME_WAIT;
    new_projectile.type = g_weapons_data.weapons[WEAPON_INDEX].projectile_type;
    new_projectile.should_remove = false;

    new_projectile.position = g_player_data.center;

    new_projectile.velocity = Vector2Subtract(g_mouse_position, new_projectile.position);
    new_projectile.velocity = Vector2Normalize(new_projectile.velocity);
    new_projectile.velocity = Vector2Scale(new_projectile.velocity, g_weapons_data.weapons[WEAPON_INDEX].start_velocity);

    if (g_weapons_data.weapons[WEAPON_INDEX].spread > 0)
    {
        float angle = rand() % g_weapons_data.weapons[WEAPON_INDEX].spread;
        angle -= g_weapons_data.weapons[WEAPON_INDEX].spread / 2;
        angle *= (rand() / (float)RAND_MAX);

        // Convert to radians
        angle *= (3.14f / 180);

        new_projectile.velocity = Vector2Rotate(new_projectile.velocity, angle);
    }
    else
    {
        new_projectile.velocity = Vector2Rotate(new_projectile.velocity, 0);
    }

    for (int i = 0; i < g_projectiles_data.player_projectile_count; i++)
    {
        if (PROJECTILE_TYPE_PLAYER_NONE == g_projectiles_data.player_projectiles[i].type)
        {
            g_projectiles_data.player_projectiles[i] = new_projectile;
            break;
        }
    }
}

void projectiles_update()
{
    for (int i = 0; i < g_projectiles_data.player_projectile_count; i++)
    {
        if (PROJECTILE_TYPE_PLAYER_NONE == g_projectiles_data.player_projectiles[i].type)
        {
            // printf("Projectile index %d - Type %d\n", i, (*projectiles)[i].type);
            continue;
        }

        if (g_projectiles_data.player_projectiles[i].should_remove)
        {
            g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
            continue;
        }

        if (0 < g_projectiles_data.player_projectiles[i].time_wait)
        {
            g_projectiles_data.player_projectiles[i].time_wait -= g_frame_time;
            // printf("Projectile index %d - Decreased time_wait to %f\n", i, (*projectiles)[i].time_wait);
            continue;
        }

        Vector2 scaled_velocity = Vector2Scale(g_projectiles_data.player_projectiles[i].velocity, g_frame_time);
        g_projectiles_data.player_projectiles[i].position = Vector2Add(g_projectiles_data.player_projectiles[i].position, scaled_velocity);

        float radius = g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius;

        // Check if out of bounds
        // Left
        if (g_projectiles_data.player_projectiles[i].position.x + radius < 0)
        {
            g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
        }

        // Right
        if (g_projectiles_data.player_projectiles[i].position.x - radius >= g_window.width)
        {
            g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
        }

        // Top
        if (g_projectiles_data.player_projectiles[i].position.y + radius < 0)
        {
            g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
        }

        // Bottom
        if (g_projectiles_data.player_projectiles[i].position.y - radius >= g_window.height)
        {
            g_projectiles_data.player_projectiles[i].type = PROJECTILE_TYPE_PLAYER_NONE;
        }

        projectile_check_collision(&g_projectiles_data.player_projectiles[i]);
    }

    for (int i = 0; i < g_projectiles_data.enemy_projectile_count; i++)
    {
        Projectile_Enemy_t *current_projectile = &g_projectiles_data.enemy_projectiles[i];
        if (PROJECTILE_TYPE_ENEMY_NONE == current_projectile->type)
        {
            continue;
        }

        if(current_projectile->type == PROJECTILE_TYPE_ENEMY_HEAVY_SHOOTER)
        {
            for (int j = 0; j < g_projectiles_data.player_projectile_count; j++)
            {
                if(g_projectiles_data.player_projectiles[j].type != PROJECTILE_TYPE_PLAYER_BURST)
                {
                    continue;
                }

                if(CheckCollisionCircles(current_projectile->position, g_projectiles_data.enemy_projectile_database[current_projectile->type].radius, g_projectiles_data.player_projectiles[j].position, g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[j].type].radius))
                {
                    current_projectile->health -= g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[j].type].damage;
                    
                    Explosion_t new_explosion = g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[j].type].explosion;
                    new_explosion.position = g_projectiles_data.player_projectiles[j].position;
                    explosion_spawn_expl(new_explosion);

                    g_projectiles_data.player_projectiles[j].type = PROJECTILE_TYPE_PLAYER_NONE;

                    if(current_projectile->health <= 0)
                    {
                        current_projectile->type = PROJECTILE_TYPE_ENEMY_NONE;
                    }
                }
            }
        }

        // printf("Enemy projectile index %d has velocity (%f, %f) and position (%f, %f)\n", i, current_projectile->velocity.x, current_projectile->velocity.y, current_projectile->position.x, current_projectile->position.y);

        current_projectile->position = Vector2Add(current_projectile->position, Vector2Scale(current_projectile->velocity, g_frame_time));

        float radius = g_projectiles_data.enemy_projectile_database[current_projectile->type].radius;

        // Check if out of bounds
        // Left
        if (current_projectile->position.x + radius < 0)
        {
            current_projectile->type = PROJECTILE_TYPE_ENEMY_NONE;
        }

        // Right
        if (current_projectile->position.x - radius >= g_window.width)
        {
            current_projectile->type = PROJECTILE_TYPE_ENEMY_NONE;
        }

        // Top
        if (current_projectile->position.y + radius < 0)
        {
            current_projectile->type = PROJECTILE_TYPE_ENEMY_NONE;
        }

        // Bottom
        if (current_projectile->position.y - radius >= g_window.height)
        {
            current_projectile->type = PROJECTILE_TYPE_ENEMY_NONE;
        }

        if (CheckCollisionCircles(current_projectile->position, g_projectiles_data.enemy_projectile_database[current_projectile->type].radius, g_player_data.center, g_player_data.hitbox_radius))
        {
            g_player_data.player_current_health -= g_projectiles_data.enemy_projectile_database[current_projectile->type].damage;
            g_projectiles_data.enemy_projectiles[i].type = PROJECTILE_TYPE_ENEMY_NONE;
        }
    }
}

void projectiles_draw()
{
    for (int i = 0; i < g_projectiles_data.player_projectile_count; i++)
    {
        if (PROJECTILE_TYPE_PLAYER_NONE == g_projectiles_data.player_projectiles[i].type)
        {
            continue;
        }

        if (0 < g_projectiles_data.player_projectiles[i].time_wait)
        {
            continue;
        }

        // DrawCircle(
        //     (*PROJECTILES)[i].position.x,
        //     (*PROJECTILES)[i].position.y,
        //     PROJECTILE_DB[(*PROJECTILES)[i].type].radius,
        //     PROJECTILE_DB[(*PROJECTILES)[i].type].color);

        switch (g_projectiles_data.player_projectiles[i].type)
        {
        case PROJECTILE_TYPE_PLAYER_NONE:
        {
            continue;
        }
        break;

        case PROJECTILE_TYPE_PLAYER_BURST:
        {
            Vector2 start_point_direction = Vector2Scale(Vector2Normalize(g_projectiles_data.player_projectiles[i].velocity), g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius);

            Vector2 start_point = Vector2Add(g_projectiles_data.player_projectiles[i].position, start_point_direction);
            Vector2 end_point = Vector2Add(g_projectiles_data.player_projectiles[i].position, Vector2Scale(start_point_direction, -1));

            DrawLineEx(start_point, end_point, 4, WHITE);
        }
        break;

        case PROJECTILE_TYPE_PLAYER_CANNON:
        {
            Vector2 first_point_direction = Vector2Scale(Vector2Normalize(g_projectiles_data.player_projectiles[i].velocity), g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius);

            Vector2 point_1 = Vector2Add(g_projectiles_data.player_projectiles[i].position, first_point_direction);
            Vector2 point_2 = Vector2Add(g_projectiles_data.player_projectiles[i].position, Vector2Rotate(first_point_direction, 240 * DEG2RAD));
            Vector2 point_3 = Vector2Add(g_projectiles_data.player_projectiles[i].position, Vector2Rotate(first_point_direction, 120 * DEG2RAD));

            DrawTriangle(point_1, point_2, point_3, WHITE);
        }
        break;

        case PROJECTILE_TYPE_PLAYER_AUTOCANNON:
        {
            Vector2 first_point_direction = Vector2Scale(Vector2Normalize(g_projectiles_data.player_projectiles[i].velocity), g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius);

            Vector2 point_1 = Vector2Add(g_projectiles_data.player_projectiles[i].position, first_point_direction);
            Vector2 point_2 = Vector2Add(g_projectiles_data.player_projectiles[i].position, Vector2Rotate(first_point_direction, 240 * DEG2RAD));
            Vector2 point_3 = Vector2Add(g_projectiles_data.player_projectiles[i].position, Vector2Rotate(first_point_direction, 120 * DEG2RAD));

            DrawTriangle(point_1, point_2, point_3, WHITE);
        }
        break;

        case PROJECTILE_TYPE_PLAYER_TORPEDO:
        {
            Vector2 rectangle_position_direction = Vector2Scale(Vector2Normalize(g_projectiles_data.player_projectiles[i].velocity), g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius);
            Vector2 rectangle_position = Vector2Rotate(rectangle_position_direction, -30 * DEG2RAD);
            rectangle_position = Vector2Add(rectangle_position, g_projectiles_data.player_projectiles[i].position);

            DrawRectanglePro(
                (Rectangle){.width = cos(240 * DEG2RAD) * 2 * g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius,
                            .height = sin(240 * DEG2RAD) * 2 * g_projectiles_data.player_projectile_database[g_projectiles_data.player_projectiles[i].type].radius,
                            .x = rectangle_position.x,
                            .y = rectangle_position.y},
                (Vector2){.x = 0, .y = 0},
                Vector2Angle((Vector2){.x = 0, .y = 1}, Vector2Normalize(g_projectiles_data.player_projectiles[i].velocity)) * RAD2DEG,
                WHITE);
        }
        break;

        default:
            DrawCircle(g_projectiles_data.player_projectiles[i].position.x, g_projectiles_data.player_projectiles[i].position.y, 50, PURPLE);
            break;
        }
    }

    for (int i = 0; i < g_projectiles_data.enemy_projectile_count; i++)
    {
        Projectile_Enemy_t *current_projectile = &g_projectiles_data.enemy_projectiles[i];

        if (current_projectile->type == PROJECTILE_TYPE_ENEMY_NONE)
        {
            continue;
        }

        switch (current_projectile->type)
        {
        case PROJECTILE_TYPE_ENEMY_SHOOTER:
        {
            DrawCircle(current_projectile->position.x, current_projectile->position.y, g_projectiles_data.enemy_projectile_database[current_projectile->type].radius, RED);
        }
        break;

        case PROJECTILE_TYPE_ENEMY_HEAVY_SHOOTER:
        {
            Vector2 rectangle_position_direction = Vector2Scale(Vector2Normalize(current_projectile->velocity), g_projectiles_data.enemy_projectile_database[current_projectile->type].radius);
            Vector2 rectangle_position = Vector2Rotate(rectangle_position_direction, -30 * DEG2RAD);
            rectangle_position = Vector2Add(rectangle_position, current_projectile->position);

            DrawRectanglePro(
                (Rectangle){.width = cos(240 * DEG2RAD) * 2 * g_projectiles_data.enemy_projectile_database[current_projectile->type].radius,
                            .height = sin(240 * DEG2RAD) * 2 * g_projectiles_data.enemy_projectile_database[current_projectile->type].radius,
                            .x = rectangle_position.x,
                            .y = rectangle_position.y},
                (Vector2){.x = 0, .y = 0},
                Vector2Angle((Vector2){.x = 0, .y = 1}, Vector2Normalize(current_projectile->velocity)) * RAD2DEG,
                PURPLE);
        }
        break;

        default:
            DrawCircle(current_projectile->position.x, current_projectile->position.y, 50, PURPLE);
            break;
        }
    }
}

void money_draw_ingame()
{
    DrawText(TextFormat("$%d", g_player_data.money + g_player_data.transaction_money_remaining), 10, 20, 40, SKYBLUE);
}

void weapons_update()
{
    for (unsigned char i = 0; i < WEAPON_TYPE_COUNT; i++)
    {
        Weapon_t *current_weapon = &g_weapons_data.weapons[i];

        if (!current_weapon->is_unlocked)
        {
            continue;
        }

        g_weapons_data.symbol_alert_timers[i] += g_frame_time;

        if (!(current_weapon->ammo_count >= current_weapon->ammo_count_max))
        {
            current_weapon->time_last_reload += g_frame_time;
        }

        if (current_weapon->time_last_reload >= current_weapon->time_ammo_reload)
        {
            current_weapon->time_last_reload -= current_weapon->time_ammo_reload;
            current_weapon->ammo_count++;
            g_weapons_data.symbol_alert_timers[current_weapon->type] = 0;
        }

        if (!IsKeyPressed(current_weapon->firing_key))
        {
            continue;
        }

        if (current_weapon->ammo_count <= 0)
        {
            continue;
        }

        current_weapon->ammo_count--;

        for (int j = 0; j < current_weapon->projectile_count; j++)
        {
            projectile_player_spawn(i, current_weapon->time_projectile_interval * j);
        }
    }
}

void game_over_screen(bool disable_buttons)
{
    DrawText("GAME OVER", g_window.width / 2 - MeasureText("GAME OVER", 40) / 2, 200, 40, RED);
    DrawText("Try again?", g_window.width / 2 - MeasureText("Try again?", 30) / 2, 275, 30, RED);

    if (create_button_text_border(
            disable_buttons,
            g_window.width * (1 / 3.0f), g_window.height * 0.65f,
            110, 70,
            SKYBLUE, true, SKYBLUE_LIGHT,
            "No", 35, BLACK, 1,
            8, BLUE))
    {
        g_gamestate_previous = g_gamestate_current;
        g_gamestate_current = STATE_UPGRADE;

        g_transition_time = 0;
        g_transition_duration = 2;
    }

    if (create_button_text_border(
            disable_buttons,
            g_window.width * (2 / 3.0f), g_window.height * 0.65f,
            110, 70,
            SKYBLUE, true, SKYBLUE_LIGHT,
            "Yes", 35, BLACK, 1,
            8, BLUE))
    {
        g_gamestate_previous = g_gamestate_current;
        g_gamestate_current = STATE_LEVEL;
        enemy_spawn_director_init_level(g_enemy_spawn_director.current_level);

        g_transition_time = 0;
        g_transition_duration = 4;
    }
}

void draw_state_selection_buttons(bool disable_buttons)
{
    short width = 50;
    short height = 50;

    Gamestate_e destinations[3] = {STATE_LEVEL_SELECTION, STATE_MAIN_MENU, STATE_UPGRADE};

    for (unsigned char i = 0; i < 3; i++)
    {
        Color color = SKYBLUE;

        int x = g_window.width / 2 + 120 * (i - 1);
        int y = g_window.height * 0.9f;

        if (g_gamestate_current == destinations[i])
        {
            color.b = 200;
            color.r = 200;

            create_button_border(
                disable_buttons,
                x, y, width + 10, height + 10, color, false, SKYBLUE_LIGHT,
                5, BLUE);

            continue;
        }

        if (create_button_border(
                disable_buttons,
                x, y, width, height, color, !disable_buttons, SKYBLUE_LIGHT,
                5, BLUE))
        {
            g_transition_duration = 0.5f;
            g_transition_time = 0;

            g_gamestate_previous = g_gamestate_current;
            g_gamestate_current = destinations[i];
        }
    }
}

void level_selector_update(bool disable_buttons, int offset)
{
    DrawText("Choose a Level:", g_window.width / 2 - MeasureText("Choose a Level:", 40) / 2 + offset, g_window.height * 0.12f, 40, SKYBLUE);

    float width_percent = 0.6f;
    float height_percent = 0.5f;

    Rectangle background = {.x = g_window.width * 0.2f + offset, .y = g_window.height * 0.3f, .width = g_window.width * width_percent, .height = g_window.height * height_percent};

    int rows = 5;
    int columns = 6;

    float button_width = g_window.width * width_percent / (columns + 1) * 0.8f;
    float button_height = g_window.height * height_percent / (rows + 1) * 0.8f;

    DrawRectangleRec(background, ORANGE);
    DrawRectangleLinesEx(background, 10, BROWN);

    int row_num = 0;
    for (int i = 0; i < g_levels_data.LEVEL_COUNT; i++)
    {
        Color button_color = SKYBLUE;
        bool can_hover = true;

        if (i > g_enemy_spawn_director.next_level)
        {
            can_hover = false;
            button_color = GRAY;
        }

        row_num = (floor(i / (float)columns)) + 1;

        if (create_button_text_border(
                disable_buttons,
                background.x + g_window.width * (width_percent / (columns + 1)) * ((i + 1) - (row_num - 1) * columns),
                background.y + g_window.height * (height_percent / (rows + 1)) * row_num,
                button_width,
                button_height,
                button_color, can_hover, SKYBLUE_LIGHT,
                TextFormat("%d", i + 1), 15, BLACK, 3,
                3, DARKBLUE))
        {
            if (i > g_enemy_spawn_director.next_level)
            {
                continue;
            }

            g_transition_time = 0;
            g_transition_duration = 4;

            g_gamestate_previous = STATE_LEVEL_SELECTION;
            enemy_spawn_director_init_level(i);
        }
    }
}

void upgrade_menu_update(bool disable_buttons, int offset)
{
    DrawText("Upgrades:", g_window.width / 2 - MeasureText("Upgrades:", 40) / 2 + offset, g_window.height * 0.08f, 40, SKYBLUE);
    DrawText(TextFormat("$%d", g_player_data.money + g_player_data.transaction_money_remaining), g_window.width / 2 - MeasureText(TextFormat("$%d", g_player_data.money + g_player_data.transaction_money_remaining), 35) / 2 + offset, g_window.height * 0.08f + 50, 35, SKYBLUE);

    if (create_button_text_border(
            disable_buttons,
            g_window.width / 2 + offset, g_window.height * 0.75f, 260, 65, SKYBLUE, true, SKYBLUE_LIGHT,
            "Next Level >>", 30, BLACK, 1,
            5, BLUE))
    {
        g_gamestate_previous = g_gamestate_current;
        enemy_spawn_director_init_level(g_enemy_spawn_director.next_level);

        g_transition_time = 0;
        g_transition_duration = 4;
    }

    for (int i = 0; i < WEAPON_TYPE_COUNT; i++)
    {
        g_weapons_data.upgrade_transitions[i] -= g_frame_time;

        bool disable_button = disable_buttons;
        int text_x = g_window.width * 0.5f + offset;
        int text_y = g_window.height * 0.25f + 60 * i;
        Color button_color = SKYBLUE;
        Color weapon_name_color = SKYBLUE;

        if (!g_weapons_data.weapons[i].is_unlocked)
        {
            weapon_name_color = (Color){.r = 30, .g = 50, .b = 75, .a = 255};
        }

        DrawText(TextFormat("%s", g_weapons_data.names[i]), 50 + offset, text_y - 15, 25, weapon_name_color);
        DrawText(TextFormat("$%d", g_weapons_data.upgrade_cost[i]), text_x - MeasureText(TextFormat("$%d", g_weapons_data.upgrade_cost[i]), 25) - 30, g_window.height * 0.25f + 60 * i - 15, 25, SKYBLUE);

        if (g_player_data.money < g_weapons_data.upgrade_cost[i] || g_weapons_data.weapons[i].level_current >= g_weapons_data.level_max)
        {
            button_color = (Color){.r = 30, .g = 50, .b = 75, .a = 255};
            disable_button = true;
        }

        if (create_button_text(disable_button, text_x, text_y, 30, 40, button_color, !disable_button, SKYBLUE_LIGHT, "+", 40, BLACK, 0))
        {
            money_remove(g_weapons_data.upgrade_cost[i]);

            if (g_weapons_data.weapons[i].is_unlocked)
            {
                g_weapons_data.upgrade_cost[i] *= g_weapons_data.cost_scaling;
                g_weapons_data.weapons[i].level_current++;

                if (g_weapons_data.upgrade_transitions[i] < 0)
                {
                    g_weapons_data.upgrade_transitions[i] = g_weapons_data.upgrade_transition_duration;
                }
                else
                {
                    g_weapons_data.upgrade_transitions[i] += g_weapons_data.upgrade_transition_duration;
                }
            }
            else
            {
                g_weapons_data.weapons[i].is_unlocked = true;
            }
        }

        for (int j = 0; j < g_weapons_data.level_max; j++)
        {
            Color indicator_color_bought = SKYBLUE;
            Color indicator_color_not_bought = (Color){.r = 30, .g = 50, .b = 75, .a = 255};

            int j_draw = (int)floor(g_weapons_data.upgrade_transitions[i] / g_weapons_data.upgrade_transition_duration);

            if (g_weapons_data.upgrade_transitions[i] > 0 && j >= g_weapons_data.weapons[i].level_current - (j_draw + 1) && j < g_weapons_data.weapons[i].level_current)
            {
                DrawRectangle(text_x + 40 + 25 * j, text_y - 20, 15, 40, indicator_color_not_bought);

                if (j != g_weapons_data.weapons[i].level_current - 1)
                {
                    continue;
                }

                DrawRectangle(text_x + 40 + 25 * (j - j_draw), text_y - 20, 15 * (1 - Clamp(g_weapons_data.upgrade_transitions[i] / g_weapons_data.upgrade_transition_duration - j_draw, 0, 1)), 40, SKYBLUE);
                continue;
            }

            if (j < g_weapons_data.weapons[i].level_current)
            {
                DrawRectangle(text_x + 40 + 25 * j, text_y - 20, 15, 40, indicator_color_bought);
                continue;
            }

            DrawRectangle(text_x + 40 + 25 * j, text_y - 20, 15, 40, indicator_color_not_bought);
        }
    }
}

void main_menu_update(int offset)
{
    g_main_menu_data.bounce_timer += (g_frame_time * g_main_menu_data.bounce_speed);
    if (g_main_menu_data.bounce_timer > PI)
    {
        g_main_menu_data.bounce_timer -= PI;
    }

    float bounce = g_main_menu_data.bounce_distance * sin(g_main_menu_data.bounce_timer);

    DrawText("- Welcome -", g_window.width / 2 - MeasureText("- Welcome -", 40) / 2 + offset, g_window.height * 0.4f, 40, SKYBLUE);

    DrawText("<-- Levels", 50 + offset + bounce, g_window.height * 0.20f, 40, SKYBLUE);
    DrawText("Upgrades -->", g_window.width - MeasureText("Upgrades ->", 40) - 50 + offset - bounce, g_window.height * 0.6f, 40, SKYBLUE);
}

void background_update()
{
    g_stars_data.twinkle_x += g_frame_time;
    for (int i = 0; i < g_stars_data.star_count; i++)
    {
        Vector2 speed = g_stars_data.star_speeds[g_stars_data.stars[i].speed_index];
        speed = Vector2Scale(speed, g_frame_time);

        g_stars_data.stars[i].position = Vector2Add(g_stars_data.stars[i].position, speed);

        if (g_stars_data.stars[i].position.x < -g_stars_data.padding_x - 10 || g_stars_data.stars[i].position.x > g_window.width + g_stars_data.padding_x + 10 || g_stars_data.stars[i].position.y > g_window.height + 10 || g_stars_data.stars[i].position.y < -10)
        {
            int chosen_position = rand_range_int(-g_stars_data.padding_x, g_window.width + g_stars_data.padding_x + g_window.height);

            if (chosen_position > g_window.width + g_stars_data.padding_x)
            {
                g_stars_data.stars[i].position = (Vector2){.x = g_window.width + g_stars_data.padding_x + 5, .y = chosen_position - g_window.width - g_stars_data.padding_x};
                continue;
            }

            g_stars_data.stars[i].position = (Vector2){.x = chosen_position, .y = -5};
            continue;
        }

        Color color = g_stars_data.star_colors[g_stars_data.stars[i].color_index];
        DrawCircle(
            g_stars_data.stars[i].position.x - (g_window.width * g_stars_data.screen_scroll),
            g_stars_data.stars[i].position.y,
            g_stars_data.stars[i].radius,
            (Color){.a = 255 * ((sin((g_stars_data.twinkle_x + g_stars_data.stars[i].twinkle_offset) * g_stars_data.twinkle_speed) + 1) / 2), .r = color.r, .g = color.g, .b = color.b});
    }
}

void explosions_draw()
{
    for (int i = 0; i < g_explosions_data.EXPLOSIONS_COUNT; i++)
    {
        if (g_explosions_data.explosions[i].time_alive >= g_explosions_data.explosions[i].time_lifetime)
        {
            continue;
        }

        DrawCircle(
            g_explosions_data.explosions[i].position.x,
            g_explosions_data.explosions[i].position.y,
            Clamp(g_explosions_data.explosions[i].time_alive / g_explosions_data.explosions[i].time_lifetime, 0, 1) * g_explosions_data.explosions[i].size,
            (Color){.a = (1 - Clamp(g_explosions_data.explosions[i].time_alive / g_explosions_data.explosions[i].time_lifetime, 0, 1)) * 255,
                    .b = g_explosions_data.explosions[i].color.b,
                    .g = g_explosions_data.explosions[i].color.g,
                    .r = g_explosions_data.explosions[i].color.r});
    }
}

void draw_player_health()
{
    DrawText(TextFormat("Station: %d/%d", g_player_data.player_current_health, g_player_data.player_max_health), g_window.width * (1 / 3.0f) - MeasureText(TextFormat("Station: %d/%d", g_player_data.player_current_health, g_player_data.player_max_health), 20) / 2, 30, 20, SKYBLUE);
    DrawText(TextFormat("Planet: %d/%d", g_player_data.planet_current_health, g_player_data.planet_max_health), g_window.width * (2 / 3.0f) - MeasureText(TextFormat("Planet: %d/%d", g_player_data.planet_current_health, g_player_data.planet_max_health), 20) / 2, 30, 20, SKYBLUE);
}

void player_update()
{
    if (IsKeyPressed(KEY_N))
    {
        g_player_data.player_current_health += 100;
    }

    if (IsKeyPressed(KEY_M))
    {
        g_player_data.planet_current_health += 100;
    }

    g_player_data.rotation += g_player_data.rotation_speed * g_frame_time;

    if (g_player_data.rotation > 360)
    {
        g_player_data.rotation -= 360;
    }

    Rectangle player_graphic = {.x = g_player_data.center.x, .y = g_player_data.center.y, .width = g_player_data.hitbox_radius * 2, .height = g_player_data.hitbox_radius * 2};

    DrawCircleV(g_player_data.center, g_player_data.hitbox_radius, g_player_data.color);
    DrawRectanglePro(player_graphic, (Vector2){.x = g_player_data.hitbox_radius, .y = g_player_data.hitbox_radius}, g_player_data.rotation, g_player_data.color);

    if (g_player_data.player_current_health <= 0)
    {
        g_player_data.color = (Color){.r = 255, .g = 0, .b = 0, .a = 0};

        g_player_data.player_current_health += 9999;
        g_player_data.planet_current_health += 9999;

        explosion_spawn(g_player_data.center, 2.5f, 250, ORANGE);

        g_gamestate_previous = g_gamestate_current;
        g_gamestate_current = STATE_GAMEOVER;

        g_transition_time = 0;
        g_transition_duration = 4;
    }

    if (g_player_data.planet_current_health <= 0)
    {
        g_player_data.player_current_health += 9999;
        g_player_data.planet_current_health += 9999;

        explosion_spawn((Vector2){.x = g_window.width / 2, .y = g_window.height}, 3.0f, 800, ORANGE);

        g_gamestate_previous = g_gamestate_current;
        g_gamestate_current = STATE_GAMEOVER;

        g_transition_time = 0;
        g_transition_duration = 4;
    }
}

void draw_cheat_keys()
{
    DrawRectangle(0, 0, g_window.width, g_window.height, BLACK);
    DrawText("L: Give 1500 money", 20, 40, 25, SKYBLUE);
    DrawText("O: Kill all enemies", 20, 80, 25, SKYBLUE);
    DrawText("P: End level", 20, 120, 25, SKYBLUE);
    DrawText("K: Hold to speed up time by 5x", 20, 160, 25, SKYBLUE);
    DrawText("J: Hold to speed up time by 10x", 20, 200, 25, SKYBLUE);
    DrawText("U: Hold to speed up time by 25x", 20, 240, 25, SKYBLUE);
    DrawText("N: Give 100 Player Health", 20, 280, 25, SKYBLUE);
    DrawText("M: Give 100 Planet Health", 20, 320, 25, SKYBLUE);
    DrawText("G: Unlock next level", 20, 360, 25, SKYBLUE);
    DrawText("T: Unlock all levels", 20, 400, 25, SKYBLUE);
    DrawText("R: Reset all levels", 20, 440, 25, SKYBLUE);
}

Color get_symbol_background_color(Weapon_Type_e type)
{
    Color color = g_weapons_data.symbol_color_default;

    if (g_weapons_data.weapons[type].ammo_count >= g_weapons_data.weapons[type].ammo_count_max)
    {
        color = g_weapons_data.symbol_color_full;
    }
    else if (g_weapons_data.weapons[type].ammo_count > 0)
    {
        color = g_weapons_data.symbol_color_not_empty;
    }

    if (!g_player_settings.weapon_ready_alert)
    {
        return color;
    }

    if ((g_weapons_data.symbol_alert_timers[type] < g_weapons_data.symbol_alert_duration * (1 / 3.0f) || g_weapons_data.symbol_alert_timers[type] > g_weapons_data.symbol_alert_duration * (2 / 3.0f)) && g_weapons_data.symbol_alert_timers[type] < g_weapons_data.symbol_alert_duration)
    {
        color = g_weapons_data.symbol_color_alert;
    }

    return color;
}

void weapons_draw_symbols()
{
    // TODO
    // Make DrawSymbol a function instead so that it is possible to draw symbols elsewhere

    const short AMMO_COUNTER_Y = g_weapons_data.symbol_draw_area_y + 15;
    const short AMMO_COUNTER_FONT_SIZE = 25;

    const unsigned char RELOAD_INDICATOR_WIDTH = 20;

    DrawRectangle(0, g_weapons_data.symbol_draw_area_y, g_window.width, g_window.height, GRAY);

    const unsigned char SYMBOL_OFFSET_Y = 20;

    for (Weapon_Type_e i = 0; i < WEAPON_TYPE_COUNT; i++)
    {
        if (!g_weapons_data.weapons[i].is_unlocked)
        {
            continue;
        }

        const int SYMBOL_X = ((float)(i + 1) / (WEAPON_TYPE_COUNT + 1)) * g_window.width - g_weapons_data.symbol_width / 2;
        const int SYMBOL_Y = g_window.height - g_weapons_data.symbol_height - SYMBOL_OFFSET_Y;

        // Symbol background
        DrawRectangle(
            SYMBOL_X,
            (g_window.height - g_weapons_data.symbol_height) - 20,
            g_weapons_data.symbol_width,
            g_weapons_data.symbol_height,
            get_symbol_background_color(i));

        // Ammo counter
        DrawText(TextFormat("%d", g_weapons_data.weapons[i].ammo_count), SYMBOL_X, AMMO_COUNTER_Y, 25, SKYBLUE);
        DrawText("|", SYMBOL_X + g_weapons_data.symbol_width / 2 - MeasureText("|", AMMO_COUNTER_FONT_SIZE), AMMO_COUNTER_Y, AMMO_COUNTER_FONT_SIZE, SKYBLUE);
        {
            const char *text = TextFormat("%d", g_weapons_data.weapons[i].ammo_count_max);
            DrawText(text, SYMBOL_X + g_weapons_data.symbol_width - MeasureText(text, AMMO_COUNTER_FONT_SIZE), AMMO_COUNTER_Y, 25, SKYBLUE);
        }

        DrawRectangle(SYMBOL_X, SYMBOL_Y + g_weapons_data.symbol_height, -RELOAD_INDICATOR_WIDTH, Clamp(g_weapons_data.weapons[i].time_last_reload / g_weapons_data.weapons[i].time_ammo_reload, 0, 1) * g_weapons_data.symbol_height * -1, GREEN);

        // Symbols
        switch (i)
        {
        case WEAPON_TYPE_BURST:
        {
            const int LINE_LENGTH = 20;
            DrawLineEx(
                (Vector2){.x = SYMBOL_X + (float)1 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + g_weapons_data.symbol_height - (g_weapons_data.symbol_height - LINE_LENGTH) / 2 + LINE_LENGTH / 2},
                (Vector2){.x = SYMBOL_X + (float)1 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + (g_weapons_data.symbol_height - LINE_LENGTH) / 2 + LINE_LENGTH / 2},
                4,
                BLACK);

            DrawLineEx(
                (Vector2){.x = SYMBOL_X + (float)2 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + g_weapons_data.symbol_height - (g_weapons_data.symbol_height - LINE_LENGTH) / 2},
                (Vector2){.x = SYMBOL_X + (float)2 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + (g_weapons_data.symbol_height - LINE_LENGTH) / 2},
                4,
                BLACK);

            DrawLineEx(
                (Vector2){.x = SYMBOL_X + (float)3 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + g_weapons_data.symbol_height - (g_weapons_data.symbol_height - LINE_LENGTH) / 2 - LINE_LENGTH / 2},
                (Vector2){.x = SYMBOL_X + (float)3 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + (g_weapons_data.symbol_height - LINE_LENGTH) / 2 - LINE_LENGTH / 2},
                4,
                BLACK);

            DrawLineEx(
                (Vector2){.x = SYMBOL_X + (float)4 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + g_weapons_data.symbol_height - (g_weapons_data.symbol_height - LINE_LENGTH) / 2},
                (Vector2){.x = SYMBOL_X + (float)4 / 5 * g_weapons_data.symbol_width, .y = SYMBOL_Y + (g_weapons_data.symbol_height - LINE_LENGTH) / 2},
                4,
                BLACK);
        }
        break;

        case WEAPON_TYPE_CANNON:
        {
            const Vector2 FIRST_POINT_DIRECTION = {.x = 0, .y = -20};
            const Vector2 TRIANGLE_CENTER = {.x = SYMBOL_X + g_weapons_data.symbol_width / 2, .y = SYMBOL_Y + g_weapons_data.symbol_height / 2};

            Vector2 point_1 = Vector2Add(TRIANGLE_CENTER, FIRST_POINT_DIRECTION);
            Vector2 point_2 = Vector2Add(TRIANGLE_CENTER, Vector2Rotate(FIRST_POINT_DIRECTION, 240 * (3.14f / 180)));
            Vector2 point_3 = Vector2Add(TRIANGLE_CENTER, Vector2Rotate(FIRST_POINT_DIRECTION, 120 * (3.14f / 180)));

            DrawTriangle(point_1, point_2, point_3, BLACK);
        }
        break;

        case WEAPON_TYPE_AUTOCANNON:
        {
            // TODO
            // Make a symbol for the autocannon
        }
        break;

        case WEAPON_TYPE_TORPEDO:
        {
            // TODO
            // Make a symbol for the torpedo
        }
        break;

        default:
            break;
        }
    }
}

void transition_update()
{
    // TODO
    // Re-factor this function

    // Instead of calling each function in each step of the transition, instead call a function to toggle specific functions when entering a new step of a transition

    g_transition_time += g_frame_time;
    g_transition_progress = Clamp(g_transition_time / g_transition_duration, 0, 1);

    switch (g_gamestate_previous)
    {
    case STATE_LEVEL:
    {
        switch (g_gamestate_current)
        {
        case STATE_GAMEOVER:
        {
            background_update();
            money_update();

            enum
            {
                FONT_SIZE = 60
            };

            if (g_transition_progress <= 0.25f)
            {
                projectiles_update();
                explosions_update();
                weapons_update();

                money_draw_ingame();
                player_update();
                projectiles_draw();
                explosions_draw();
                enemies_update();
                enemies_draw();
                weapons_draw_symbols();

                DrawText(
                    TextFormat("Level %d Failed...", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d Failed...", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * g_transition_progress * 4, .r = RED.r, .g = RED.g, .b = RED.b});
            }
            else if (g_transition_progress <= 0.5f)
            {
                projectiles_update();
                explosions_update();
                weapons_update();

                money_draw_ingame();
                player_update();
                projectiles_draw();
                explosions_draw();
                enemies_update();
                enemies_draw();
                weapons_draw_symbols();

                DrawText(
                    TextFormat("Level %d Failed...", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d Failed...", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    RED);
            }
            else if (g_transition_progress <= 0.75)
            {
                projectiles_update();
                explosions_update();
                weapons_update();

                money_draw_ingame();
                player_update();
                projectiles_draw();
                explosions_draw();
                enemies_update();
                enemies_draw();
                weapons_draw_symbols();

                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (g_transition_progress - 0.5f) * 4, .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
                DrawText(
                    TextFormat("Level %d Failed...", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d Failed...", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (1.0f - ((g_transition_progress - 0.5f) * 4)), .r = RED.r, .g = RED.g, .b = RED.b});
            }
            else if (g_transition_progress < 1.0f)
            {
                game_over_screen(true);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (1 - (g_transition_progress - 0.75f) * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
        }
        break;

        case STATE_UPGRADE:
        {
            background_update();

            enum
            {
                FONT_SIZE = 60
            };

            if (g_transition_progress <= 0.25f)
            {
                projectiles_update();
                explosions_update();
                weapons_update();

                money_draw_ingame();
                player_update();
                projectiles_draw();
                explosions_draw();
                weapons_draw_symbols();

                DrawText(
                    TextFormat("Level %d Cleared", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d Cleared", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * g_transition_progress * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
            else if (g_transition_progress <= 0.5f)
            {
                projectiles_update();
                explosions_update();
                weapons_update();

                money_draw_ingame();
                player_update();
                projectiles_draw();
                explosions_draw();
                weapons_draw_symbols();

                DrawText(
                    TextFormat("Level %d Cleared", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d Cleared", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    BLUE);
            }
            else if (g_transition_progress <= 0.75)
            {
                projectiles_update();
                explosions_update();
                weapons_update();

                money_draw_ingame();
                player_update();
                projectiles_draw();
                explosions_draw();
                weapons_draw_symbols();

                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (g_transition_progress - 0.5f) * 4, .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
                DrawText(
                    TextFormat("Level %d Cleared", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d Cleared", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (1.0f - ((g_transition_progress - 0.5f) * 4)), .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
            else if (g_transition_progress < 1.0f)
            {
                g_stars_data.screen_scroll = 0.5f;
                upgrade_menu_update(false, 0);
                draw_state_selection_buttons(true);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (1 - (g_transition_progress - 0.75f) * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
        }
        break;

        default:
            break;
        }
    }
    break;

    case STATE_LEVEL_SELECTION:
    {
        switch (g_gamestate_current)
        {
        case STATE_LEVEL:
        {

            background_update();

            enum
            {
                FONT_SIZE = 60
            };

            if (g_transition_progress <= 0.25f)
            {
                draw_state_selection_buttons(true);
                level_selector_update(true, 0);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (g_transition_progress * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
            else if (g_transition_progress <= 0.5f)
            {
                g_stars_data.screen_scroll = 0;
                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (1.0f - (g_transition_progress - 0.25f) * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (g_transition_progress - 0.25f) * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
            else if (g_transition_progress <= 0.75)
            {
                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    BLUE);
            }
            else if (g_transition_progress < 1.0f)
            {
                // TODO
                // Sometimes at the end of this section of the transition, the level label occasionally pops up for a split second

                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (1.0f - (g_transition_progress - 0.75f)) * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
        }
        break;

        case STATE_MAIN_MENU:
        {
            float scroll_factor = (sin((PI / 2 - PI) + PI * g_transition_progress) + 1) / 2;
            g_stars_data.screen_scroll = -0.5f + 0.5f * scroll_factor;
            background_update();
            draw_state_selection_buttons(true);

            level_selector_update(true, -g_window.width * scroll_factor);
            main_menu_update(g_window.width * (1 - scroll_factor));
        }
        break;

        case STATE_UPGRADE:
        {
            float scroll_factor = (sin((PI / 2 - PI) + PI * g_transition_progress) + 1) / 2;
            g_stars_data.screen_scroll = -0.5f + scroll_factor;
            background_update();
            draw_state_selection_buttons(true);

            level_selector_update(true, -g_window.width * scroll_factor * 2);
            main_menu_update(g_window.width * (1 - scroll_factor * 2));
            upgrade_menu_update(true, g_window.width * (2 - scroll_factor * 2));
        }

        default:
            break;
        }
    }
    break;

    case STATE_MAIN_MENU:
    {
        switch (g_gamestate_current)
        {
        case STATE_LEVEL_SELECTION:
        {
            float scroll_factor = (sin((PI / 2 - PI) + PI * g_transition_progress) + 1) / 2;
            g_stars_data.screen_scroll = -0.5f * scroll_factor;
            background_update();
            draw_state_selection_buttons(true);

            level_selector_update(true, -g_window.width * (1 - scroll_factor));
            main_menu_update(g_window.width * scroll_factor);
        }
        break;

        case STATE_UPGRADE:
        {
            float scroll_factor = (sin((PI / 2 - PI) + PI * g_transition_progress) + 1) / 2;
            g_stars_data.screen_scroll = 0.5f * scroll_factor;
            background_update();
            draw_state_selection_buttons(true);

            main_menu_update(-g_window.width * scroll_factor);
            upgrade_menu_update(true, g_window.width * (1 - scroll_factor));
        }
        break;

        default:
            break;
        }
    }
    break;

    case STATE_UPGRADE:
    {
        switch (g_gamestate_current)
        {
        case STATE_MAIN_MENU:
        {
            float scroll_factor = (sin((PI / 2 - PI) + PI * g_transition_progress) + 1) / 2;
            g_stars_data.screen_scroll = 0.5f * (1 - scroll_factor);
            background_update();
            draw_state_selection_buttons(true);

            main_menu_update(-g_window.width * (1 - scroll_factor));
            upgrade_menu_update(true, g_window.width * scroll_factor);
        }

        break;
        case STATE_LEVEL_SELECTION:
        {
            float scroll_factor = (sin((PI / 2 - PI) + PI * g_transition_progress) + 1) / 2;
            g_stars_data.screen_scroll = 0.5 - scroll_factor;
            background_update();
            draw_state_selection_buttons(true);

            level_selector_update(true, -g_window.width * (2 - scroll_factor * 2));
            main_menu_update(-g_window.width * (1 - scroll_factor * 2));
            upgrade_menu_update(true, g_window.width * scroll_factor * 2);
        }
        break;

        case STATE_LEVEL:
        {
            background_update();

            enum
            {
                FONT_SIZE = 60
            };

            if (g_transition_progress <= 0.25f)
            {
                draw_state_selection_buttons(true);
                upgrade_menu_update(true, 0);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (g_transition_progress * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
            else if (g_transition_progress <= 0.5f)
            {
                draw_player_health();
                g_stars_data.screen_scroll = 0;
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (1.0f - (g_transition_progress - 0.25f) * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (g_transition_progress - 0.25f) * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
            else if (g_transition_progress <= 0.75)
            {
                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    BLUE);
            }
            else if (g_transition_progress < 1.0f)
            {
                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (1.0f - (g_transition_progress - 0.75f)) * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
        }
        break;

        default:
            break;
        }
    }
    break;

    case STATE_GAMEOVER:
    {
        switch (g_gamestate_current)
        {
        case STATE_LEVEL:
        {
            background_update();

            enum
            {
                FONT_SIZE = 60
            };

            if (g_transition_progress <= 0.25f)
            {
                game_over_screen(true);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (g_transition_progress * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
            else if (g_transition_progress <= 0.5f)
            {
                draw_player_health();
                g_stars_data.screen_scroll = 0;
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (1.0f - (g_transition_progress - 0.25f) * 4), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (g_transition_progress - 0.25f) * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
            else if (g_transition_progress <= 0.75)
            {
                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    BLUE);
            }
            else if (g_transition_progress < 1.0f)
            {
                draw_player_health();
                money_draw_ingame();
                player_update();
                weapons_draw_symbols();
                DrawText(
                    TextFormat("Level %d", g_enemy_spawn_director.current_level + 1),
                    g_window.width / 2 - MeasureText(TextFormat("Level %d", g_enemy_spawn_director.current_level + 1), FONT_SIZE) / 2,
                    g_window.height * 0.35f,
                    FONT_SIZE,
                    (Color){.a = 255 * (1.0f - (g_transition_progress - 0.75f)) * 4, .r = BLUE.r, .g = BLUE.g, .b = BLUE.b});
            }
        }
        break;

        case STATE_UPGRADE:
        {
            background_update();

            enum
            {
                FONT_SIZE = 60
            };

            if (g_transition_progress <= 0.5f)
            {
                game_over_screen(true);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (g_transition_progress * 2), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
            else if (g_transition_progress < 1.0f)
            {
                g_stars_data.screen_scroll = 0.5f;
                upgrade_menu_update(false, 0);
                draw_state_selection_buttons(true);
                DrawRectangle(0, 0, g_window.width, g_window.height, (Color){.a = 255 * (1 - (g_transition_progress - 0.5f) * 2), .r = BLACK.r, .g = BLACK.g, .b = BLACK.b});
            }
        }
        break;

        default:
            break;
        }
    }

    default:
        break;
    }
}

void update_current_state()
{
    switch (g_gamestate_current)
    {
    case STATE_LEVEL:
    {
        money_update();

        g_stars_data.screen_scroll = 0;
        background_update();
        player_update();

        weapons_update();
        enemies_update();
        projectiles_update();
        explosions_update();
        enemies_update_spawn_conditions();

        // DrawText(TextFormat("%d", GetFPS()), 50, 50, 40, WHITE);
        projectiles_draw();

        enemies_draw();
        explosions_draw();

        weapons_draw_symbols();

        draw_player_health();
        money_draw_ingame();
    }
    break;

    case STATE_MAIN_MENU:
    {
        g_stars_data.screen_scroll = 0;

        background_update();
        draw_state_selection_buttons(false);
        main_menu_update(0);
    }
    break;

    case STATE_LEVEL_SELECTION:
    {
        g_stars_data.screen_scroll = -0.5f;

        background_update();
        draw_state_selection_buttons(false);
        level_selector_update(false, 0);
    }
    break;

    case STATE_UPGRADE:
    {
        money_update();
        g_stars_data.screen_scroll = 0.5f;
        background_update();
        draw_state_selection_buttons(false);
        upgrade_menu_update(false, 0);
    }
    break;

    case STATE_GAMEOVER:
    {
        background_update();
        game_over_screen(false);
    }
    break;

    default:
        break;
    }
}

//--------------------------------------------------

int main()
{
    player_init();
    explosions_init();
    projectiles_init();
    enemies_init();
    background_init();

    g_weapons_data.symbol_draw_area_y = g_window.height - 125;

    // SetTargetFPS(60);
    srand(time(NULL));

    InitWindow(g_window.width, g_window.height, "Arcade Project");

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_L))
        {
            money_add(1000);
        }

        g_frame_time = GetFrameTime();
        if (IsKeyDown(KEY_K))
        {
            g_frame_time *= 5;
        }
        else if (IsKeyDown(KEY_J))
        {
            g_frame_time *= 10;
        }
        else if (IsKeyDown(KEY_U))
        {
            g_frame_time *= 25;
        }

        if(IsKeyPressed(KEY_G))
        {
            g_enemy_spawn_director.next_level++;

            if(g_enemy_spawn_director.next_level > g_enemy_spawn_director.max_level)
            {
                g_enemy_spawn_director.next_level = g_enemy_spawn_director.max_level;
            }
        }

        if(IsKeyPressed(KEY_T))
        {
            g_enemy_spawn_director.next_level = g_enemy_spawn_director.max_level;
        }

        if(IsKeyPressed(KEY_R))
        {
            g_enemy_spawn_director.next_level = 0;
        }
        

        g_mouse_position = GetMousePosition();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Hold H to show cheats", 20, g_window.height - 30, 20, SKYBLUE);

        if (g_transition_time < g_transition_duration)
        {
            transition_update();

            if (IsKeyDown(KEY_H))
            {
                draw_cheat_keys();
            }

            EndDrawing();

            continue;
        }

        update_current_state();

        if (IsKeyDown(KEY_H))
        {
            draw_cheat_keys();
        }

        EndDrawing();
    }
}