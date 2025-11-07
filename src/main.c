#include "game.h"
#include <dlfcn.h>
#include <raylib.h>
#include <stdio.h>

void *game = NULL;
game_init_t *game_init = NULL;
game_update_t *game_update = NULL;
game_pre_reload_t *game_pre_reload = NULL;
game_post_reload_t *game_post_reload = NULL;

void reload() {
  void *state = NULL;
  if (game != NULL) {
    state = game_pre_reload();
    dlclose(game);
  }

  game = dlopen("build/game.so", RTLD_NOW);
  game_init = (game_init_t *)dlsym(game, "game_init");
  game_update = (game_update_t *)dlsym(game, "game_update");
  game_pre_reload = (game_pre_reload_t *)dlsym(game, "game_pre_reload");
  game_post_reload = (game_post_reload_t *)dlsym(game, "game_post_reload");

  if (state != NULL) {
    game_post_reload(state);
  }
}

int main(int argc, char **argv) {
  reload();

  game_init();
  InitWindow(800, 600, "New Game");
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      reload();
    }
    BeginDrawing();
    game_update();
    EndDrawing();
  }
}
