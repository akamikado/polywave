#include "game.h"
#include <complex.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

#define FFT_SIZE (1 << 13)

typedef struct {
  char *song;
  Wave wave;
  Music music;

  float in_raw[FFT_SIZE];
  float in_smooth[FFT_SIZE];
  float complex out_raw[FFT_SIZE];
  float out[FFT_SIZE];
  float out_smooth[FFT_SIZE];
} State;

static State *s = NULL;

static void audio_callback(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;

  for (unsigned int i = 0; i < frames; i++) {
    memmove(s->in_raw, s->in_raw + 1, (FFT_SIZE - 1) * sizeof(s->in_raw[0]));
    s->in_raw[FFT_SIZE - 1] = fs[i][0];
  }
}

static inline float amp(float complex z) {
  float zabs = cabs(z);
  return logf(zabs * zabs);
}

static void fft_clean() {
  memset(s->in_raw, 0, sizeof(s->in_raw));
  memset(s->in_smooth, 0, sizeof(s->in_smooth));
  memset(s->out_raw, 0, sizeof(s->out_raw));
  memset(s->out, 0, sizeof(s->out));
  memset(s->out_smooth, 0, sizeof(s->out_smooth));
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

void fft_render(Rectangle bbox) {
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
  float max_amp = 1.0f;
  for (float f = lowf; (size_t)f < FFT_SIZE / 2; f = ceilf(f * step)) {
    float f1 = ceilf(f * step);
    float a = 0.0f;
    for (size_t q = (size_t)f; q < FFT_SIZE / 2 && q < (size_t)f1; ++q) {
      float b = amp(s->out_raw[q]);
      if (b > a)
        a = b;
    }
    if (max_amp < a)
      max_amp = a;
    s->out[out_cnt++] = a;
  }

  // Normalize
  for (size_t i = 0; i < out_cnt; ++i) {
    s->out[i] /= max_amp;
  }

  int smoothness = 15;
  float dt = GetFrameTime();
  for (size_t i = 0; i < out_cnt; ++i) {
    s->out_smooth[i] += (s->out[i] - s->out_smooth[i]) * smoothness * dt;
  }

  float rec_width = bbox.width / out_cnt;

  for (int i = 0; i < out_cnt; i++) {
    Vector2 start_pos = {.x = bbox.x + rec_width * i,
                         .y = bbox.y + bbox.height -
                              bbox.height * s->out_smooth[i]};
    Vector2 size = {.x = rec_width, .y = bbox.height * s->out_smooth[i]};
    DrawRectangleV(start_pos, size, RED);
  }
}

void game_init() {
  SetTargetFPS(60);
  s = malloc(sizeof(State));
  InitAudioDevice();
  s->song = strdup("build/In-the-Dead-of-Night.ogg");
  /* s->song = strdup("build/Insane-Gameplay.mp3"); */
  s->wave = LoadWave(s->song);
  s->music = LoadMusicStream(s->song);
  PlayMusicStream(s->music);
  AttachAudioStreamProcessor(s->music.stream, audio_callback);
}

void game_update() {
  ClearBackground(BLACK);
  if (IsMusicStreamPlaying(s->music)) {
    UpdateMusicStream(s->music);

    float w = GetScreenWidth();
    float h = GetScreenHeight();
    Rectangle bbox = {.x = 0, .y = 100, .width = w, .height = h - 200};
    fft_render(bbox);
  }
}

void *game_pre_reload() {
  fft_clean();
  return s;
}

void game_post_reload(void *prevState) { s = prevState; }

void game_close() { CloseAudioDevice(); }
