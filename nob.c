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

  cmd_append(&cmd, "cc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-I./raylib-5.5_linux_amd64/include/");
  cmd_append(&cmd, "-g");
  cmd_append(&cmd, "-shared", "-fPIC");
  cmd_append(&cmd, "-o", BUILD_FOLDER "game.so", SRC_FOLDER "game.c");
  cmd_append(&cmd, "-L./raylib-5.5_linux_amd64/lib/");
  cmd_append(&cmd, "-l:libraylib.so");
  cmd_append(&cmd, "-lm");
  if (!cmd_run(&cmd)) {
    fprintf(stderr, "Failed created game.so\n");
    return 1;
  }

  cmd_append(&cmd, "cc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-I./raylib-5.5_linux_amd64/include/");
  cmd_append(&cmd, "-g");
  cmd_append(&cmd, "-o", BUILD_FOLDER "main", SRC_FOLDER "main.c");
  cmd_append(&cmd, "-Wl,-rpath=./raylib-5.5_linux_amd64/lib/");
  cmd_append(&cmd, "-L./raylib-5.5_linux_amd64/lib/");
  cmd_append(&cmd, "-l:libraylib.so");
  cmd_append(&cmd, "-lm");
  if (!cmd_run(&cmd)) {
    fprintf(stderr, "Failed creating main executable\n");
    return 1;
  }

  return 0;
}
