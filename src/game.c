#include "game.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *song;
  Wave wave;
  Music music;
} State;

static State *s = NULL;

static void audio_callback(void *bufferData, unsigned int frames) {}

void game_init() {
  s = malloc(sizeof(State));
  InitAudioDevice();
  s->song = strdup("build/Insane-Gameplay.mp3");
  s->wave = LoadWave(s->song);
  s->music = LoadMusicStream(s->song);
  PlayMusicStream(s->music);
  AttachAudioStreamProcessor(s->music.stream, audio_callback);
}

void game_update() {
  ClearBackground(BLACK);
  if (IsMusicStreamPlaying(s->music)) {
    UpdateMusicStream(s->music);

    float *data = LoadWaveSamples(s->wave);
  }
}

void *game_pre_reload() { return s; }

void game_post_reload(void *prevState) { s = prevState; }

void game_close() { CloseAudioDevice(); }
