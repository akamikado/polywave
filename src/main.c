#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

#include <raylib.h>

#include "game.h"
#include "config.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  game_init();
  InitWindow(WNDW_WIDTH, WNDW_HEIGHT, "New Game");
  while (!WindowShouldClose()) {
    BeginDrawing();
    if (!game_update()) break;
    EndDrawing();
  }
  CloseWindow();
  game_close();
}
