#ifdef _WIN32
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
/*#define NONLS             // All NLS defines and routines*/
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#define MMNOSOUND

typedef struct tagMSG *LPMSG;

#include <windows.h>

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

/* @raysan5: Some required types defined for MSVC/TinyC compiler */
#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif
#endif

#if defined(_WIN32)
	#undef near
	#undef far
#endif

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

  #ifdef _WIN32
  cmd_append(&cmd, "gcc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-mwindows");
  cmd_append(&cmd, "-I./raylib-5.5_win64_mingw-w64/include");
  cmd_append(&cmd, "-o", BUILD_FOLDER "polywave-windows.exe");
  cmd_append(&cmd, SRC_FOLDER "main.c");
  cmd_append(&cmd, SRC_FOLDER "game.c");
  cmd_append(&cmd, "-L./raylib-5.5_win64_mingw-w64/lib");
  cmd_append(&cmd, "-lraylib");
  cmd_append(&cmd, "-lm");
  cmd_append(&cmd, "-lwinmm");
  #else
  #ifdef MACOS
  cmd_append(&cmd, "clang");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-I./raylib-5.5_macos/include/");
  cmd_append(&cmd, "-o", BUILD_FOLDER "polywave-linux");
  cmd_append(&cmd, SRC_FOLDER "main.c");
  cmd_append(&cmd, SRC_FOLDER "game.c");
  cmd_append(&cmd, "./raylib-5.5_macos/lib/libraylib.a");
  cmd_append(&cmd, "-lm");
  #else
  cmd_append(&cmd, "cc");
  cmd_append(&cmd, "-Wall", "-Wextra");
  cmd_append(&cmd, "-I./raylib-5.5_linux_amd64/include/");
  cmd_append(&cmd, "-o", BUILD_FOLDER "polywave-linux");
  cmd_append(&cmd, SRC_FOLDER "main.c");
  cmd_append(&cmd, SRC_FOLDER "game.c");
  cmd_append(&cmd, "./raylib-5.5_linux_amd64/lib/libraylib.a");
  cmd_append(&cmd, "-lm");
  #endif
  #endif

  if (!cmd_run(&cmd)) {
    fprintf(stderr, "Failed creating main executable\n");
    return 1;
  }

  return 0;
}
