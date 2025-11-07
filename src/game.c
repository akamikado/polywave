#include "game.h"
#include <raylib.h>
#include <stdlib.h>

typedef struct {
} State;

static State *s = NULL;

void game_init() { s = malloc(sizeof(State)); }

void game_update() { ClearBackground(BLUE); }

void *game_pre_reload() { return s; }

void game_post_reload(void *prevState) { s = prevState; }
