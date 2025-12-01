#include <complex.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "config.h"

#include <raylib.h>
#include <raymath.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#ifdef _WIN32
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
/*#define NONLS             // All NLS defines and routines*/
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#define MMNOSOUND

typedef struct tagMSG *LPMSG;

#include <windows.h>

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

/* @raysan5: Some required types defined for MSVC/TinyC compiler */
#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif
#endif

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "../nob.h"

#define GAME_TIME (10.0f * 60.0f) // 10 MINS
#define TIMER_COLOR GetColor(0xb7978fff)
#define TIMER_THICKNESS 20.0f

#define FFT_SIZE (1 << 13)
#define NUM_BARS 40
#define VISUALIZER_COLOR GetColor(0xdc5318ff)

#define LOAD_RADIUS 750.0f

#define OBJ_COLOR GetColor(0x523a34ff)
#define OBJ_SIZE 60.0f
#define OBJ_GEN_START_TIME 20.0f

#define CHUNK_SIZE (3 * 3 * OBJ_SIZE)
#define MAX_CHUNKS (size_t)(0.25*1024*1024 / sizeof(Map_Chunk))

#define INIT_FUEL_PER_CHUNK 5
#define MAX_FUEL_PER_CHUNK 35
#define FUEL_SIZE 4.0f
#define FUEL_COLOR GetColor(0x0e34a0ff)
#define FUEL_PICKUP_REWARD 0.05f

#define ENEMY_COLOR GetColor(0xfb232eff)
#define ENEMY_SIZE 60.0f
#define MAX_ENEMIES 25
#define ENEMY_SPD 180.0f
#define ENEMY_DEFEAT_REWARD 0.8f
#define ENEMY_GEN_START_TIME 40.0f
#define INIT_ENEMY_GEN_RATE 0.5f
#define MAX_ENEMY_GEN_RATE 0.05f

#define PLAYER_COLOR GetColor(0xef946cff)
#define PLAYER_NORMAL_SPD 225.0f
#define PLAYER_BOOST_SPD 750.0f
#define PLAYER_SIZE 40.0f
#define BOOST_TIME 600.0f

typedef struct {
  Vector2 center;
  bool obj_generated;
  struct {
    int i, j;
  } obj;
  struct {
    Vector2* items;
    size_t count;
    size_t capacity;
  } fuel;
} Map_Chunk;

typedef struct Map_Chunk_List {
  struct Map_Chunk_List* next;
  struct Map_Chunk_List* prev;
  Map_Chunk* value;
} Map_Chunk_List;

typedef struct {
  Vector2 pos;
  float rotation;
} Enemy;

typedef struct {
  Enemy *items;
  size_t count;
  size_t capacity;
} Enemies;

typedef enum {
  GAME_PLAYING,
  GAME_LOST,
  GAME_WON
} Game_State;

typedef struct {
  bool displaying;
  Game_State state;

  float time;

  char *song;
  Wave wave;
  Music music;

  float in_raw[FFT_SIZE];
  float in_smooth[FFT_SIZE];
  float complex out_raw[FFT_SIZE];
  float out[FFT_SIZE];
  size_t out_cnt;

  Camera2D camera;

  float cursor_size;

  Vector2 character_pos;
  float character_size;
  Vector2 character_speed;
  bool boost_mode;
  float boost_speed;
  float fuel_percent;

  struct {
    Vector2 key;
    Map_Chunk_List* value;
  }* chunk_loaded; 
  struct {
    Map_Chunk_List* head, *tail;
  } chunks;
  size_t fuel_per_chunk;


  Enemies enemies;
  size_t max_enemies;
  float last_enemy_gen_time;
  float enemy_gen_rate;
} State;

static State *s = NULL;

static void init_chunks() {
  s->chunk_loaded = NULL;
  s->chunks.head = malloc(sizeof(*(s->chunks.head)));
  s->chunks.tail = malloc(sizeof(*(s->chunks.tail)));
  s->chunks.head->next = s->chunks.tail;
  s->chunks.head->prev = NULL;
  s->chunks.tail->next = NULL;
  s->chunks.tail->prev = s->chunks.head;
}

static void destroy_chunks() {
  hmfree(s->chunk_loaded);
  free(s->chunks.head);
  free(s->chunks.tail);
}

static void unload_chunk() {
  assert(hmlen(s->chunk_loaded) > 0);
  Map_Chunk_List* least_priority_chunk = s->chunks.tail->prev;

  least_priority_chunk->prev->next = s->chunks.tail;
  s->chunks.tail->prev = least_priority_chunk->prev;

  assert(hmdel(s->chunk_loaded, least_priority_chunk->value->center) == 1);
  free(least_priority_chunk->value);
  free(least_priority_chunk);
}

static void generate_chunk(Vector2 center, bool generate_object) {
  if ((size_t)hmlen(s->chunk_loaded) + 1 > MAX_CHUNKS) {
    unload_chunk();
  }

  Map_Chunk* chunk = malloc(sizeof(*chunk));
  chunk->center = center;
  chunk->obj_generated = false;
  chunk->obj.i = -1;
  chunk->obj.j = -1;
  
  if (generate_object && (rand() % 5 < 3) /*60% chance to generate*/) {
    bool objects[3][3];
    memset(objects, true, sizeof(objects));

    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        if (i == 0 && j == 0) continue;

        Vector2 adj_chunk_center = Vector2Add(center, (Vector2){.x = i * CHUNK_SIZE, .y = j * CHUNK_SIZE});
        if (hmgeti(s->chunk_loaded, adj_chunk_center) < 0) continue;

        Map_Chunk* adj_chunk = hmget(s->chunk_loaded, adj_chunk_center)->value;

        if (i < 0) {
          if (j < 0) {
            if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 2) objects[0][0] = false;
          } else if (j == 0) {
            if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 2) objects[0][0] = false;
            else if (adj_chunk->obj.i == 1 && adj_chunk->obj.j == 2) objects[1][0] = 0;
            else if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 2) objects[2][0] = 0;
          } else if (j > 0) {
            if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 2) objects[2][0] = 0;
          }
        } else if (i == 0) {
          if (j < 0) {
            if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 0) objects[0][0] = 0;
            else if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 1) objects[0][1] = 0;
            else if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 2) objects[0][2] = 0;
          } else if (j > 0) {
            if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 0) objects[2][0] = 0;
            else if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 1) objects[2][1] = 0;
            else if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 2) objects[2][2] = 0;
          }
        } else {
          if (j < 0) {
            if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 0) objects[0][2] = 0;
          } else if (j == 0) {
            if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 0) objects[0][2] = 0;
            else if (adj_chunk->obj.i == 1 && adj_chunk->obj.j == 0) objects[1][2] = 0;
            else if (adj_chunk->obj.i == 2 && adj_chunk->obj.j == 0) objects[2][2] = 0;
          } else if (j > 0) {
            if (adj_chunk->obj.i == 0 && adj_chunk->obj.j == 0) objects[2][2] = 0;
          }
        }
      }
    }

    int remaining = 0;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        remaining += objects[i][j];
      }
    }

    int chosen = (rand() % remaining) + 1, k = 0;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (objects[i][j]) k++;
        if (chosen == k) {
          chunk->obj.i = i;
          chunk->obj.j = j;
        }
      }
    }

    chunk->obj_generated = true;
  }

  size_t num_fuel = rand() % s->fuel_per_chunk;
  Vector2 corner = Vector2Subtract(center, (Vector2){.x = CHUNK_SIZE / 2, .y = CHUNK_SIZE / 2});
  memset(&chunk->fuel, 0, sizeof(chunk->fuel));

  for(size_t i = 0; i < num_fuel; i++) {
    Vector2 pos = {
      .x = corner.x + rand() % (size_t)CHUNK_SIZE,
      .y = corner.y + rand() % (size_t)CHUNK_SIZE
    };

    da_append(&chunk->fuel, pos);
  }

  Map_Chunk_List *head = s->chunks.head, *next = head->next;
  Map_Chunk_List* new = malloc(sizeof(*new));
  new->value = chunk;

  head->next = new;
  new->prev = head;

  new->next = next;
  next->prev = new;

  hmput(s->chunk_loaded, center, new);
}

static void update_chunk_priority(Vector2 key) {
  if (hmlen(s->chunk_loaded) <= 1) return;

  Map_Chunk_List* chunk = hmget(s->chunk_loaded, key);

  chunk->prev->next = chunk->next;
  chunk->next->prev = chunk->prev;

  s->chunks.head->next->prev = chunk;
  chunk->next = s->chunks.head->next;

  s->chunks.head->next = chunk;
  chunk->prev = s->chunks.head;
}

static void load_chunks() {
  s->fuel_per_chunk = INIT_FUEL_PER_CHUNK + (MAX_FUEL_PER_CHUNK - INIT_FUEL_PER_CHUNK) * (s->time / GAME_TIME);

  for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
      Vector2 center_of_rec;
      center_of_rec.x = floorf((s->character_pos.x + i * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;
      center_of_rec.y = floorf((s->character_pos.y + j * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;

      if (hmgeti(s->chunk_loaded, center_of_rec) < 0) {
        generate_chunk(center_of_rec, s->time > OBJ_GEN_START_TIME);
      } else {
        update_chunk_priority(center_of_rec);
      }
    }
  }
}

void draw_chunk() {
  for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
      Vector2 center_of_rec;
      center_of_rec.x = floorf((s->character_pos.x + i * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;
      center_of_rec.y = floorf((s->character_pos.y + j * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;

      assert(hmgeti(s->chunk_loaded, center_of_rec) >= 0);
      Map_Chunk* chunk = hmget(s->chunk_loaded, center_of_rec)->value;
      if (chunk->obj_generated){
        Vector2 obj_center = Vector2Add(center_of_rec, (Vector2){.x = (chunk->obj.i - 1) * OBJ_SIZE, .y = (chunk->obj.j - 1) * OBJ_SIZE});
        Vector2 obj_corner = Vector2Subtract(obj_center, (Vector2){.x = OBJ_SIZE / 2, .y = OBJ_SIZE / 2});
        DrawRectangleV(obj_corner, (Vector2){.x = OBJ_SIZE, .y = OBJ_SIZE}, OBJ_COLOR);

        #ifdef DISPLAY_COLLISION_LINES
          Vector2 verts[] = {
            {.x = obj_center.x - OBJ_SIZE * 0.55f, .y = obj_center.y - OBJ_SIZE * 0.55f},
            {.x = obj_center.x - OBJ_SIZE * 0.55f, .y = obj_center.y + OBJ_SIZE * 0.55f},
            {.x = obj_center.x + OBJ_SIZE * 0.55f, .y = obj_center.y + OBJ_SIZE * 0.55f},
            {.x = obj_center.x + OBJ_SIZE * 0.55f, .y = obj_center.y - OBJ_SIZE * 0.55f},
          };
          DrawLineV(verts[0], verts[1], WHITE);
          DrawLineV(verts[1], verts[2], WHITE);
          DrawLineV(verts[2], verts[3], WHITE);
          DrawLineV(verts[3], verts[0], WHITE);
       #endif

      }

      for (size_t i = 0; i < chunk->fuel.count; i++) {
          DrawCircleV(chunk->fuel.items[i], FUEL_SIZE, FUEL_COLOR);
      }
    }
  }
}

static void calc_player_speed(float dt, Vector2 mouse_dist) {
  if (Vector2Length(mouse_dist) > PLAYER_SIZE) {
    if (s->boost_mode) {
      Vector2 mouse_dir = Vector2Normalize(mouse_dist);
      s->character_speed.x = s->boost_speed * mouse_dir.x;
      s->character_speed.y = s->boost_speed * mouse_dir.y;
    } else {
      s->character_speed.x = PLAYER_NORMAL_SPD * (mouse_dist.x / (WNDW_WIDTH / 2));
      s->character_speed.y = PLAYER_NORMAL_SPD * (mouse_dist.y / (WNDW_HEIGHT / 2));
    }

    Vector2 new_pos = Vector2Add(s->character_pos, Vector2Scale(s->character_speed, dt));

    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        Vector2 center_of_rec;
        center_of_rec.x = floorf((new_pos.x + i * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;
        center_of_rec.y = floorf((new_pos.y + j * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;

        if(hmgeti(s->chunk_loaded, center_of_rec) < 0) continue;

        Map_Chunk* chunk = hmget(s->chunk_loaded, center_of_rec)->value;
        if (!chunk->obj_generated) continue;

        Vector2 obj_center = Vector2Add(center_of_rec, (Vector2){.x = (chunk->obj.i - 1) * OBJ_SIZE, .y = (chunk->obj.j - 1) * OBJ_SIZE});
        Vector2 obj_corner = Vector2Subtract(obj_center, (Vector2){.x = OBJ_SIZE / 2, .y = OBJ_SIZE / 2});
        Rectangle obj = {
          .x = obj_corner.x,
          .y = obj_corner.y,
          .width = OBJ_SIZE,
          .height = OBJ_SIZE
        };

        float factor = 1.1;
        Vector2 verts[] = {
          {.x = obj_center.x - OBJ_SIZE * 0.5 * factor, .y = obj_center.y - OBJ_SIZE * 0.5 * factor},
          {.x = obj_center.x - OBJ_SIZE * 0.5 * factor, .y = obj_center.y + OBJ_SIZE * 0.5 * factor},
          {.x = obj_center.x + OBJ_SIZE * 0.5 * factor, .y = obj_center.y + OBJ_SIZE * 0.5 * factor},
          {.x = obj_center.x + OBJ_SIZE * 0.5 * factor, .y = obj_center.y - OBJ_SIZE * 0.5 * factor},
        };

        if (CheckCollisionCircleLine(new_pos, PLAYER_SIZE, verts[0], verts[1])) {
          if (s->character_speed.x > 0) s->character_speed.x = 0;
        } 
        if (CheckCollisionCircleLine(new_pos, PLAYER_SIZE, verts[1], verts[2])) {
          if (s->character_speed.y < 0) s->character_speed.y = 0;
        } 
        if (CheckCollisionCircleLine(new_pos, PLAYER_SIZE, verts[2], verts[3])) {
          if (s->character_speed.x < 0) s->character_speed.x = 0;
        } 
        if (CheckCollisionCircleLine(new_pos, PLAYER_SIZE, verts[3], verts[0])) {
          if (s->character_speed.y > 0) s->character_speed.y = 0;
        } 
        if (CheckCollisionPointCircle(verts[0], s->character_pos, PLAYER_SIZE)) {
          if (s->character_speed.x > 0 && s->character_speed.y > 0) {
            s->character_speed = Vector2Zero();
          }
        } 
        if (CheckCollisionPointCircle(verts[1], s->character_pos, PLAYER_SIZE)) {
          if (s->character_speed.x > 0 && s->character_speed.y < 0) {
            s->character_speed = Vector2Zero();
          }
        } 
        if (CheckCollisionPointCircle(verts[2], s->character_pos, PLAYER_SIZE)) {
          if (s->character_speed.x < 0 && s->character_speed.y < 0) {
            s->character_speed = Vector2Zero();
          }
        } 
        if (CheckCollisionPointCircle(verts[3], s->character_pos, PLAYER_SIZE)) {
          if (s->character_speed.x < 0 && s->character_speed.y > 0) {
            s->character_speed = Vector2Zero();
          }
        }
        if (CheckCollisionPointRec(new_pos, obj)) {
          Vector2 dir = Vector2Normalize(Vector2Subtract(s->character_pos, obj_center));
          if (dir.x == 0 && dir.y == 0) {
            dir = (Vector2) {.x = 1, .y = 0};
          }
          s->character_speed.x = dir.x * PLAYER_NORMAL_SPD;
          s->character_speed.y = dir.y * PLAYER_NORMAL_SPD;
        }
      }
    }
  } else {
    s->character_speed = Vector2Zero();
  }
}

static void calc_player_pos(float dt, Vector2 mouse_rel) {
  calc_player_speed(dt, mouse_rel);

  s->character_pos = Vector2Add(s->character_pos, Vector2Scale(s->character_speed, dt));
}

static void generate_enemies() {
  if (s->time < ENEMY_GEN_START_TIME) return;

  for (size_t i = 0; i < s->enemies.count; i++) {
    if (!CheckCollisionPointCircle(s->enemies.items[i].pos, s->character_pos, LOAD_RADIUS)){
      da_remove_unordered(&s->enemies, i);
    }
  }

  s->enemy_gen_rate = INIT_ENEMY_GEN_RATE + (MAX_ENEMY_GEN_RATE - INIT_ENEMY_GEN_RATE) * (s->time / GAME_TIME);

  // TODO: change this while loop to create enemies after certain period of time
  if (s->time - s->last_enemy_gen_time > s->enemy_gen_rate && s->enemies.count < s->max_enemies) {
    int spawn_pos_angle = (rand() % 360) * (M_PI / 180.0f);
    Vector2 spawn_pos = {.x = s->character_pos.x + LOAD_RADIUS * cosf(spawn_pos_angle), .y = s->character_pos.y + LOAD_RADIUS * sinf(spawn_pos_angle)};
    da_append(&s->enemies, ((Enemy){.pos = spawn_pos, .rotation = 0}));
    s->last_enemy_gen_time = s->time;
  }
}

void calc_enemy_position(float dt) {
  size_t GRID_SIZE = 5 * (CHUNK_SIZE / OBJ_SIZE);
  bool occupied[GRID_SIZE][GRID_SIZE];
  memset(occupied, 0, sizeof(occupied));

  for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
      Vector2 center_of_rec;
      center_of_rec.x = floorf((s->character_pos.x + i * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;
      center_of_rec.y = floorf((s->character_pos.y + j * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;

      assert(hmgeti(s->chunk_loaded, center_of_rec) >= 0);

      Map_Chunk* chunk = hmget(s->chunk_loaded, center_of_rec)->value;
      occupied[(int)((2 + i) * (CHUNK_SIZE / OBJ_SIZE) + chunk->obj.i)][(int)((2 + j) * (CHUNK_SIZE / OBJ_SIZE) + chunk->obj.j)] = true;
    }
  }

  typedef struct {
    int x, y;
  } Cell;

  Vector2 player_chunk = {.x = floorf(s->character_pos.x / CHUNK_SIZE) * CHUNK_SIZE, .y = floorf(s->character_pos.y / CHUNK_SIZE) * CHUNK_SIZE};
  Vector2 grid_corner = {.x = player_chunk.x - 2 * CHUNK_SIZE, .y = player_chunk.y - 2 * CHUNK_SIZE};
  Cell player_cell = {.x = floor((s->character_pos.x - grid_corner.x) / OBJ_SIZE), .y = floor((s->character_pos.y - grid_corner.y) / OBJ_SIZE)};


  int distance[GRID_SIZE][GRID_SIZE];
  for (size_t i = 0; i < GRID_SIZE; i++) {
    for (size_t j = 0; j < GRID_SIZE; j++) {
      distance[i][j] = -1; 
    }
  }

  struct {
    Cell* items;
    size_t count;
    size_t capacity;
  } queue = {0};

  bool visited[GRID_SIZE][GRID_SIZE];
  memset(visited, 0, sizeof(visited));

  da_append(&queue, player_cell);
  visited[player_cell.y][player_cell.x] = true;
  distance[player_cell.y][player_cell.x] = 0;

  int neighbors[8][2] = {
    {1, 0}, {-1, 0},
    {0, 1}, {0, -1},
    {1, 1}, {-1, 1},
    {1, -1}, {-1, -1}
  };

  while (queue.count > 0) {
    Cell c = queue.items[0];
    da_remove_unordered(&queue, 0);

    for (int k=0;k<8;k++) {
      int nx = c.x + neighbors[k][0];
      int ny = c.y + neighbors[k][1];

      if (nx < 0 || ny < 0 || (size_t)nx >= GRID_SIZE || (size_t)ny >= GRID_SIZE)
        continue;

      if (occupied[ny][nx])
        continue;

      if (!visited[ny][nx]) {
        visited[ny][nx] = true;
        distance[ny][nx] = distance[c.y][c.x] + 1;
        da_append(&queue, ((Cell){nx, ny}));
      }
    }
  }

  for (size_t i = 0; i < s->enemies.count; i++) {
    Cell enemy_cell = {
      .x = floor((s->enemies.items[i].pos.x - grid_corner.x) / OBJ_SIZE),
      .y = floor((s->enemies.items[i].pos.y - grid_corner.y) / OBJ_SIZE),
    };

    Cell best = enemy_cell;
    int best_dist = distance[enemy_cell.y][enemy_cell.x];

    for (int k = 0; k < 8; k++) {
      int nx = enemy_cell.x + neighbors[k][0];
      int ny = enemy_cell.y + neighbors[k][1];

      if (nx < 0 || ny < 0 || (size_t)nx >= GRID_SIZE || (size_t)ny >= GRID_SIZE)
        continue;

      if (distance[ny][nx] != -1 && distance[ny][nx] < best_dist) {
        best_dist = distance[ny][nx];
        best.x = nx;
        best.y = ny;
      }
    }

    Vector2 target = {
      .x = grid_corner.x + best.x * OBJ_SIZE + OBJ_SIZE * 0.5f,
      .y = grid_corner.y + best.y * OBJ_SIZE + OBJ_SIZE * 0.5f
    };
    Vector2 dir = Vector2Subtract(target, s->enemies.items[i].pos);
    dir = Vector2Normalize(dir);

    Vector2 separation = {0, 0};
    float desired = 40.0f;


    for (size_t j=0; j<s->enemies.count; j++) {
        if (i == j) continue;

        Vector2 diff = Vector2Subtract(s->enemies.items[i].pos, s->enemies.items[j].pos);
        float d = Vector2Length(diff);

        if (d > 0 && d < desired) {
            Vector2 push = Vector2Scale(Vector2Normalize(diff),
                                        (desired - d) / desired);
            separation = Vector2Add(separation, push);
        }
    }
    separation = Vector2Normalize(Vector2Scale(separation, 50.0f));

    Vector2 velocity = Vector2Scale(Vector2Normalize(Vector2Add(dir, separation)), ENEMY_SPD);
    Vector2 new_pos = Vector2Add(s->enemies.items[i].pos, Vector2Scale(velocity, dt));

    Cell new_cell = {
      .x = floor((new_pos.x - grid_corner.x) / OBJ_SIZE),
      .y = floor((new_pos.y - grid_corner.y) / OBJ_SIZE)
    };

    if (new_cell.x >= 0 && 
      (size_t)new_cell.x < GRID_SIZE && 
      new_cell.y >= 0 && 
      (size_t)new_cell.y < GRID_SIZE && 
      !occupied[new_cell.y][new_cell.x]){
        s->enemies.items[i].pos = new_pos;
    } else {
      Vector2 alt = s->enemies.items[i].pos;

      Vector2 testX = { s->enemies.items[i].pos.x + velocity.x * dt,
                        s->enemies.items[i].pos.y };
      Cell alt_cell = {
        .x = floor((testX.x - grid_corner.x) / OBJ_SIZE),
        .y = floor((testX.y - grid_corner.y) / OBJ_SIZE)
      };
      if (alt_cell.x >= 0 && 
          (size_t)alt_cell.x < GRID_SIZE && 
          alt_cell.y >= 0 && 
          (size_t)alt_cell.y < GRID_SIZE &&
          !occupied[alt_cell.y][alt_cell.x]){
          alt = testX;
        }

      Vector2 testY = { s->enemies.items[i].pos.x,
                        s->enemies.items[i].pos.y + velocity.y * dt };
      alt_cell = (Cell) {
        .x = floor((testY.x - grid_corner.x) / OBJ_SIZE),
        .y = floor((testY.y - grid_corner.y) / OBJ_SIZE)
      };
      if (alt_cell.x >= 0 && 
          (size_t)alt_cell.x < GRID_SIZE && 
          alt_cell.y >= 0 && 
          (size_t)alt_cell.y < GRID_SIZE &&
          !occupied[alt_cell.y][alt_cell.x]) {
          alt = testY;
        }

      s->enemies.items[i].pos = alt;
    }


    Vector2 dist_to_player = Vector2Subtract(s->character_pos, s->enemies.items[i].pos);
    float angle = atan2(dist_to_player.y, dist_to_player.x);
    s->enemies.items[i].rotation = (angle * 180.0 / M_PI) - 90.0f;
  }
}

void draw_enemies() {
  for (size_t i = 0; i < s->enemies.count; i++) {
    double r = ENEMY_SIZE / sqrt(3.0);
    double rot = s->enemies.items[i].rotation * M_PI / 180.0;
    double angles_deg[3] = { 90.0, 210.0, 330.0 };

    Vector2 verts[3];
    for (int j = 0; j < 3; j++) {
      double a = (angles_deg[j] * M_PI / 180.0) + rot;
      verts[j].x = s->enemies.items[i].pos.x + r * cos(a);
      verts[j].y = s->enemies.items[i].pos.y + r * sin(a);
    }

    DrawTriangle(verts[2], s->enemies.items[i].pos, verts[0], ENEMY_COLOR);
    DrawTriangle(verts[0], s->enemies.items[i].pos, verts[1], ENEMY_COLOR);
  }
}

void check_enemy_collision() {
  for (size_t i = 0; i < s->enemies.count; i++) {
    double r = ENEMY_SIZE / sqrt(3.0);
    double rot = s->enemies.items[i].rotation * M_PI / 180.0;
    double angles_deg[3] = { 90.0, 210.0, 330.0 };

    Vector2 verts[3];
    for (int j = 0; j < 3; j++) {
      double a = (angles_deg[j] * M_PI / 180.0) + rot;
      verts[j].x = s->enemies.items[i].pos.x + r * cos(a);
      verts[j].y = s->enemies.items[i].pos.y + r * sin(a);
    }

    if (s->boost_mode) {
      if (CheckCollisionCircleLine(s->character_pos, PLAYER_SIZE, verts[0], verts[1]) ||
        CheckCollisionCircleLine(s->character_pos, PLAYER_SIZE, verts[1], verts[2]) ||
        CheckCollisionCircleLine(s->character_pos, PLAYER_SIZE, verts[2], verts[0]) ||
        CheckCollisionPointTriangle(s->character_pos, verts[0], verts[1], verts[2])) {
        da_remove_unordered(&s->enemies, i);
        s->fuel_percent += ENEMY_DEFEAT_REWARD;
        s->fuel_percent = s->fuel_percent > 1 ? 1 : s->fuel_percent;
      }
    } else {
      if (CheckCollisionPointCircle(verts[0], s->character_pos, PLAYER_SIZE)) {
        s->state = GAME_LOST;
      }
    }
  }
}

void check_fuel_collision() {
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      Vector2 center_of_rec;
      center_of_rec.x = floorf((s->character_pos.x + i * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;
      center_of_rec.y = floorf((s->character_pos.y + j * CHUNK_SIZE)/CHUNK_SIZE)*CHUNK_SIZE;

      if(hmgeti(s->chunk_loaded, center_of_rec) < 0) continue;

      Map_Chunk* chunk = hmget(s->chunk_loaded, center_of_rec)->value;

      size_t i = 0;
      while (i < chunk->fuel.count) {
        if (CheckCollisionCircles(
            s->character_pos, PLAYER_SIZE, chunk->fuel.items[i], FUEL_SIZE)) {
          s->fuel_percent += FUEL_PICKUP_REWARD;
          s->fuel_percent = s->fuel_percent > 1 ? 1 : s->fuel_percent;
          chunk->fuel.items[i] = chunk->fuel.items[chunk->fuel.count - 1];
          chunk->fuel.count--;
        } else {
          i++;
        }
      }
    }
  }
}

void calc_boost_speed() {
  size_t amp_per_bar = floor(s->out_cnt / NUM_BARS);
  float sum = 0.0f;
  for (int i = 0; i < floor(NUM_BARS * s->fuel_percent); i++) {
    float avg_amp = 0.0f;
    for (size_t j = 0; i * amp_per_bar + j < s->out_cnt && j < amp_per_bar; j++) {
      avg_amp += s->out[i * amp_per_bar + j];
    }
    avg_amp /= amp_per_bar;
    sum += avg_amp;
  }
  sum = sum > 1 ? 1 : sum;
  s->boost_speed = sum * PLAYER_BOOST_SPD;
  s->boost_speed = s->boost_speed < PLAYER_NORMAL_SPD ? PLAYER_NORMAL_SPD : s->boost_speed;
}

static void audio_callback(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;

  for (unsigned int i = 0; i < frames; i++) {
    memmove(s->in_raw, s->in_raw + 1, (FFT_SIZE - 1) * sizeof(s->in_raw[0]));
    s->in_raw[FFT_SIZE - 1] = fs[i][0];
  }
}

static void fft_clean() {
  memset(s->in_raw, 0, sizeof(s->in_raw));
  memset(s->in_smooth, 0, sizeof(s->in_smooth));
  memset(s->out_raw, 0, sizeof(s->out_raw));
  memset(s->out, 0, sizeof(s->out));
}

// https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm#Data_reordering,_bit_reversal,_and_in-place_algorithms
static void fft(float *a, float complex *A) {
  unsigned long n = FFT_SIZE;
  for (size_t i = 0; i < n; i++) {
    A[i] = a[i];
  }

  for (size_t i = 1, j = 0; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;
    if (i < j) {
      float complex temp = A[i];
      A[i] = a[j];
      A[j] = temp;
    }
  }

  for (size_t m = 2; m <= n; m <<= 1) {
    float complex w_m = cexp(2 * M_PI * I / m);
    for (size_t k = 0; k < n; k += m) {
      float complex w = 1;
      for (size_t j = 0; j < m / 2; j++) {
        float complex t = w * A[k + j + m / 2];
        float complex u = A[k + j];
        A[k + j] = u + t;
        A[k + j + m / 2] = u - t;
        w = w * w_m;
      }
    }
  }
}

void fft_analyze() {
  // https://en.wikipedia.org/wiki/Hann_function
  for (size_t i = 0; i < FFT_SIZE; ++i) {
    float t = (float)i / FFT_SIZE;
    float hann = 0.5 - 0.5 * cosf(2 * M_PI * t);
    s->in_smooth[i] = s->in_raw[i] * hann;
  }

  fft(s->in_smooth, s->out_raw);

  float step = 1.059463094359;
  float lowf = 1.0f;
  size_t out_cnt = 0;
  float max_amp = 1e-6f;
  float smoothness = 0.85f;
  float amplify = 1.0f;

  for (float f = lowf; (size_t)f < FFT_SIZE / 2; f = ceilf(f * step)) {
    float f1 = ceilf(f * step);
    float a = 0.0f;
    float sum = 0.0f;
    size_t cnt = 0;
    for (size_t q = (size_t)f; q < FFT_SIZE / 2 && q < (size_t)f1; ++q) {
      // Amplitude
      float b = cabsf(s->out_raw[q]);
      /* if (b > a) { */
      /*   a = b; */
      /* } */
      sum += b * b;
      /* sum += b; */
      cnt++;
    }
    /* a = cnt > 0 ? sum / cnt : 0; */
    a = cnt > 0 ? sqrtf(sum / cnt) : 0;
    a = log1pf(a * amplify);

    s->out[out_cnt] = s->out[out_cnt] * smoothness + a * (1.0f - smoothness);
    if (max_amp < s->out[out_cnt])
      max_amp = s->out[out_cnt];
    out_cnt++;
  }

  // Normalize
  for (size_t i = 0; i < out_cnt; ++i) {
    s->out[i] /= max_amp;
    s->out[i] = (1 - cosf(M_PI * s->out[i])) / 2.0;
  }

  s->out_cnt = out_cnt;
}

void fft_render(Rectangle bbox, float displayable) {
  size_t amp_per_bar = floor(s->out_cnt / NUM_BARS);
  float rec_width = bbox.width / (NUM_BARS * 2);
  float gap_between_bars = bbox.width / (NUM_BARS * 2);

  for (int i = 0; i < floor(NUM_BARS * displayable); i++) {
    float avg_amp = 0.0f;
    for (size_t j = 0; i * amp_per_bar + j < s->out_cnt && j < amp_per_bar; j++) {
      avg_amp += s->out[i * amp_per_bar + j];
    }
    avg_amp /= amp_per_bar;

    Vector2 start_pos = {.x = bbox.x + (gap_between_bars + rec_width) * i,
                         .y = bbox.y + bbox.height - bbox.height * avg_amp};
    Vector2 size = {.x = rec_width, .y = bbox.height * avg_amp};
    DrawRectangleV(start_pos, size, VISUALIZER_COLOR);
  }
}

void game_init(char* music_file) {
  srand(time(NULL));

  SetTargetFPS(60);

  s = malloc(sizeof(State));
  memset(s, 0, sizeof(*s)); 

  s->displaying = true;
  s->state = GAME_PLAYING;

  InitAudioDevice();
  char song[1024];
  sprintf(song, "assets/%s", music_file);
  s->song = strdup(song);

  s->wave = LoadWave(s->song);
  s->music = LoadMusicStream(s->song);
  fft_clean();
  PlayMusicStream(s->music);
  AttachAudioStreamProcessor(s->music.stream, audio_callback);

  s->fuel_percent = 1.0f;
  s->fuel_per_chunk = INIT_FUEL_PER_CHUNK;

  s->camera.offset = (Vector2){.x = WNDW_WIDTH / 2, .y = WNDW_HEIGHT / 2};
  s->camera.target = s->character_pos;
  s->camera.rotation = 0.0f;
  s->camera.zoom = 1.0f;

  s->cursor_size = 10.0f;

  init_chunks();

  s->max_enemies = MAX_ENEMIES;
  s->enemy_gen_rate = INIT_ENEMY_GEN_RATE;
  memset(&s->enemies, 0, sizeof(s->enemies));
}

void camera_init() {
  s->camera.offset = (Vector2){.x = WNDW_WIDTH / 2, .y = WNDW_HEIGHT / 2};
  s->camera.target = s->character_pos;
}

bool game_update() {
  float w = GetScreenWidth();
  float h = GetScreenHeight();
  float dt = GetFrameTime();

  HideCursor();

  ClearBackground(BG_COLOR);

  switch (s->state) {
    case GAME_PLAYING:
      {
        s->time += dt;

        if (s->time >= GAME_TIME) {
          s->state = GAME_WON;
        }

        camera_init();

        if (IsMusicStreamPlaying(s->music)) {
          UpdateMusicStream(s->music);
          fft_analyze();
        }

        Vector2 mouse = GetMousePosition();
        Vector2 mouse_center_relative = Vector2Subtract(mouse, (Vector2){.x = WNDW_WIDTH / 2, .y = WNDW_HEIGHT / 2});
        Vector2 mouse_character_relative = Vector2Add(mouse_center_relative, s->character_pos);

        if(IsKeyDown(KEY_SPACE) && s->fuel_percent > 0) {
          if (!s->boost_mode) {
            calc_boost_speed();
          }
          s->boost_mode = true;
          s->fuel_percent -= dt;
        } else {
          s->boost_mode = false;
        }

        calc_player_pos(dt, mouse_center_relative);

        load_chunks();

        generate_enemies();
        calc_enemy_position(dt);

        check_enemy_collision();

        check_fuel_collision();

        BeginMode2D(s->camera);

        draw_chunk();
        draw_enemies();

        #ifdef DISPLAY_LOAD_RADIUS
          DrawCircleLinesV(s->character_pos, LOAD_RADIUS, WHITE);
        #endif

        DrawCircleV(s->character_pos, PLAYER_SIZE, PLAYER_COLOR);
        DrawCircleV(mouse_character_relative, s->cursor_size, PLAYER_COLOR);

        if (s->time < OBJ_GEN_START_TIME) {
          const char* game_aim = "Survive till timer reaches end";
          const char* boost_help = "Press SPACE for boost";
          DrawText(game_aim, -MeasureText(game_aim, 50)/2, 100, 50, TEXT_COLOR);
          DrawText(boost_help, -MeasureText(boost_help, 50)/2, 200, 50, TEXT_COLOR);
        }

        EndMode2D();

        if (IsMusicStreamPlaying(s->music)) {
          Rectangle bbox = {.x = w - 225, .y = h - 175, .width = 200, .height = 150};
          fft_render(bbox, s->fuel_percent);
        }

        float timer_length = (s->time / GAME_TIME) * w;
        DrawLineEx((Vector2){.x = 0, .y = h}, (Vector2){.x = timer_length, .y = h}, TIMER_THICKNESS, TIMER_COLOR);
      }
      break;
    case GAME_WON:
      {
        if (IsKeyPressed(KEY_ENTER)) {
          s->displaying = false;
        }

        const char* win_message = "YOU WON!!";
        DrawText(win_message, w / 2 - MeasureText(win_message, 75) / 2, h / 2, 75, TEXT_COLOR);
        const char* continue_message = "Press ENTER to return to main screen";
        DrawText(continue_message, w / 2 - MeasureText(continue_message, 50) / 2, h / 2 + 200, 50, TEXT_COLOR);
      }
      break;
    case GAME_LOST:
      {
        if (IsKeyPressed(KEY_ENTER)) {
          s->displaying = false;
        }

        const char* loss_message = "YOU LOST";
        DrawText(loss_message, w / 2 - MeasureText(loss_message, 75) / 2, h / 2 - 100, 75, TEXT_COLOR);
        const char* better_luck_message = "Better luck next time!";
        DrawText(better_luck_message, w / 2 - MeasureText(better_luck_message, 75) / 2, h / 2, 75, TEXT_COLOR);
        const char* continue_message = "Press ENTER to return to main screen";
        DrawText(continue_message, w / 2 - MeasureText(continue_message, 50) / 2, h / 2 + 200, 50, TEXT_COLOR);
      }
      break;
  }

  return s->displaying;
}

void game_close() { 
  destroy_chunks();
  free(s);
  CloseAudioDevice();
}
