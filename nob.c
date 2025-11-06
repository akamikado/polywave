#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_FOLDER "build/"

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  Cmd cmd = {0};

  cmd_append(&cmd, "mkdir", "-p", "build");
  if (!cmd_run(&cmd))
    return 1;

  cmd_append(&cmd, "cc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-I./raylib-5.5_linux_amd64/include/");
  cmd_append(&cmd, "-o", BUILD_FOLDER "main", "main.c");
  cmd_append(&cmd, "-l:libraylib.a");
  cmd_append(&cmd, "-L./raylib-5.5_linux_amd64/lib/");
  cmd_append(&cmd, "-lm");
  if (!cmd_run(&cmd))
    return 1;

  cmd_append(&cmd, BUILD_FOLDER "main");
  if (!cmd_run(&cmd))
    return 1;
  return 0;
}
