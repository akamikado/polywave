#include "game.h"
#include <raylib.h>
#include <stdlib.h>

typedef struct {
  Wave w;
} State;

static State *s = NULL;

void game_init() {
  s = malloc(sizeof(State));
  InitAudioDevice();
  s->w = LoadWave("build/Insane-Gameplay.mp3");
  Sound music = LoadSoundFromWave(s->w);
  PlaySound(music);
}

void game_update() { ClearBackground(BLUE); }

void *game_pre_reload() { return s; }

void game_post_reload(void *prevState) { s = prevState; }

void game_close() { CloseAudioDevice(); }
