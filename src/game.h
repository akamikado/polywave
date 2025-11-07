#ifndef GAME_H_
#define GAME_H_

typedef void(game_init_t)();
typedef void(game_update_t)();
typedef void *(game_pre_reload_t)();
typedef void(game_post_reload_t)(void *prevState);

#endif // GAME_H_
