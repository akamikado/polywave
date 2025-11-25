#include <stdlib.h>
#include "game.h"
#include <dlfcn.h>
#include <raylib.h>
#include <stdio.h>

void *game = NULL;

#define X(name, ...) name##_t *name = NULL;
GAME_VARS
#undef X

void reload() {
  void *state = NULL;
  if (game != NULL) {
    state = game_pre_reload();
    dlclose(game);
  }

  game = dlopen("build/game.so", RTLD_NOW);
  char* e = dlerror();
  if (e != NULL) {
    fprintf(stderr, "%s\n", e);
    exit(1);
  }
  game_init = (game_init_t *)dlsym(game, "game_init");
  game_update = (game_update_t *)dlsym(game, "game_update");
  game_pre_reload = (game_pre_reload_t *)dlsym(game, "game_pre_reload");
  game_post_reload = (game_post_reload_t *)dlsym(game, "game_post_reload");
  game_close = (game_close_t *)dlsym(game, "game_close");

  if (state != NULL) {
    game_post_reload(state);
  }
}

int main(int argc, char **argv) {
  reload();

  game_init();
  InitWindow(WNDW_WIDTH, WNDW_HEIGHT, "New Game");
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      reload();
    }
    BeginDrawing();
    game_update();
    EndDrawing();
  }
  CloseWindow();
  game_close();
}
