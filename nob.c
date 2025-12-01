#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include <stdio.h>

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  Cmd cmd = {0};

  if (!mkdir_if_not_exists("build")) {
    fprintf(stderr, "Creating build directory failed\n");
    return 1;
  }

  #ifdef __WIN32
  cmd_append(&cmd, "x86_64-w64-mingw32-gcc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-mwindows");
  cmd_append(&cmd, "-I./raylib-5.5_win32_mingw-w64/include/");
  cmd_append(&cmd, "-o", BUILD_FOLDER "polywave.exe");
  cmd_append(&cmd, SRC_FOLDER "main.c");
  cmd_append(&cmd, SRC_FOLDER "game.c");
  cmd_append(&cmd, "-L./raylib-5.5_win32_mingw-w64/lib/");
  cmd_append(&cmd, "-lraylib");
  cmd_append(&cmd, "-lm");
  if (!cmd_run(&cmd)) {
    fprintf(stderr, "Failed creating main executable\n");
    return 1;
  }
  #else
  cmd_append(&cmd, "cc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-I./raylib-5.5_linux_amd64/include/");
  cmd_append(&cmd, "-o", BUILD_FOLDER "polywave");
  cmd_append(&cmd, SRC_FOLDER "main.c");
  cmd_append(&cmd, SRC_FOLDER "game.c");
  cmd_append(&cmd, "./raylib-5.5_linux_amd64/lib/libraylib.a");
  cmd_append(&cmd, "-lm");
  if (!cmd_run(&cmd)) {
    fprintf(stderr, "Failed creating main executable\n");
    return 1;
  }
  #endif

  return 0;
}
