#include "game.h"
#include <complex.h>
#include <math.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

#define FFT_SIZE (1 << 13)

typedef struct {
  char *song;
  Wave wave;
  Music music;

  float *in_raw;
  float *out;
} State;

static State *s = NULL;

static void audio_callback(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;

  for (unsigned int i = 0; i < frames; i++) {
    memmove(s->in_raw, s->in_raw + 1, (FFT_SIZE - 1) * sizeof(s->in_raw[0]));
    s->in_raw[FFT_SIZE - 1] = fs[i][0];
  }
}

// https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm#Data_reordering,_bit_reversal,_and_in-place_algorithms
static void fft(float *a, float complex *A) {
  unsigned long n = FFT_SIZE;
  for (size_t k = 0; k < n; k++) {
    size_t count = sizeof(k) * 8 - 1;
    size_t rev_k = k;
    size_t copy_k = k;

    copy_k >>= 1;
    while (copy_k) {
      rev_k <<= 1;
      rev_k |= copy_k & 1;
      copy_k >>= 1;
      count--;
    }
    rev_k <<= count;

    A[rev_k] = a[k];
  }

  for (size_t m = 2; m <= n; m <<= 1) {
    float complex w_m = cexp(2 * M_PI * I / m);
    for (size_t k = 0; k <= n - 1 / m; k++) {
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
  }
}

void *game_pre_reload() { return s; }

void game_post_reload(void *prevState) { s = prevState; }

void game_close() { CloseAudioDevice(); }
