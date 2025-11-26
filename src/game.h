#ifndef GAME_H_
#define GAME_H_

#define WNDW_WIDTH  1200
#define WNDW_HEIGHT 900

#define GAME_VARS                                                              \
  X(game_init, void, void)                                                     \
  X(game_update, void, void)                                                   \
  X(game_pre_reload, void *, void)                                             \
  X(game_post_reload, void, void *prevState)                                   \
  X(game_close, void, void)

#define X(name, ret, ...) typedef ret(name##_t)(__VA_ARGS__);
GAME_VARS
#undef X

#endif // GAME_H_
