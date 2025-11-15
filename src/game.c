#include "game.h"
#include <complex.h>
#include <math.h>
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

  float gravity_acceleration;

  Vector2 character_pos;
  Vector2 character_size;
  Vector2 character_speed;
} ReloadableState;

static ReloadableState *s = NULL;

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
  }

  int num_bars = 30;
  int amp_per_bar = floor(out_cnt / num_bars);
  float rec_width = bbox.width / (num_bars * 2);
  float gap_between_bars = bbox.width / (num_bars * 2);

  for (int i = 0; i < num_bars; i++) {
    float avg_amp = 0.0f;
    for (int j = 0; i * amp_per_bar + j < out_cnt && j < amp_per_bar; j++) {
      avg_amp += s->out[i * amp_per_bar + j];
    }
    avg_amp /= amp_per_bar;

    Vector2 start_pos = {.x = bbox.x + (gap_between_bars + rec_width) * i,
                         .y = bbox.y + bbox.height - bbox.height * avg_amp};
    Vector2 size = {.x = rec_width, .y = bbox.height * avg_amp};
    DrawRectangleV(start_pos, size, RED);
  }
}

#define PLAYER_HORIZONTAL_SPD 350
#define PLAYER_VERTICAL_SPD 500

void game_init() {
  SetTargetFPS(60);
  s = malloc(sizeof(ReloadableState));
  InitAudioDevice();
  s->song = strdup("build/bfg_division.mp3");
  /* s->song = strdup("build/In-the-Dead-of-Night.ogg"); */
  /* s->song = strdup("build/Dark-Lands.mp3"); */
  /* s->song = strdup("build/Insane-Gameplay.mp3"); */
  s->wave = LoadWave(s->song);
  s->music = LoadMusicStream(s->song);
  fft_clean();
  /* PlayMusicStream(s->music); */
  AttachAudioStreamProcessor(s->music.stream, audio_callback);

  s->gravity_acceleration = -1000;

  s->character_pos = (Vector2){.x = 100, .y = 100};
  s->character_size = (Vector2){.x = 50, .y = 50};
  s->character_speed = (Vector2){.x = 0, .y = 0};
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

  float dt = GetFrameTime();

  if (IsKeyPressed(KEY_SPACE)) {
    s->character_speed.y = -PLAYER_VERTICAL_SPD;
  }

  if (IsKeyDown(KEY_D) || IsKeyPressed(KEY_D)) {
    s->character_speed.x = PLAYER_HORIZONTAL_SPD;
  } else if (IsKeyPressed(KEY_A) || IsKeyDown(KEY_A)) {
    s->character_speed.x = -PLAYER_HORIZONTAL_SPD;
  } else {
    s->character_speed.x = 0;
  }

  s->character_speed.y -= s->gravity_acceleration * dt;

  s->character_pos.x += s->character_speed.x * dt;
  s->character_pos.y += s->character_speed.y * dt;

  DrawRectangleV(Vector2Subtract(s->character_pos,
                                 Vector2Divide(s->character_size,
                                               (Vector2){.x = 2, .y = 2})),
                 s->character_size, RED);
}

void *game_pre_reload() {
  fft_clean();
  return s;
}

void game_post_reload(void *prevState) { s = prevState; }

void game_close() { CloseAudioDevice(); }
