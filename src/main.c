#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
#else
#include <dirent.h>
#endif

#if defined(_WIN32)
	#undef near
	#undef far
#endif

#include "../nob.h"

#include <raylib.h>

#include "game.h"
#include "config.h"

#define SELECT_OPTION_COLOR GetColor(0xef946cff)

typedef enum {
  MENU,
  MUSIC_MENU,
  PLAYING
} Screen;

int main() {
  Screen current_screen = MENU;


  struct {
    char** items;
    size_t count;
    size_t capacity;
  } music_files = {0};
  int list_offset = 0;

  int selected_option = 0;

  InitWindow(WNDW_WIDTH, WNDW_HEIGHT, "New Game");
  while (!WindowShouldClose()) {
    BeginDrawing();
    switch (current_screen) {
      case MENU: 
        {
          ClearBackground(BG_COLOR);

          const char *title = "POLYWAVE";
          const char *options[] = { "PLAY", "EXIT" };
          const int option_count = 2;

          if (IsKeyPressed(KEY_UP)) {
              selected_option--;
              if (selected_option < 0) selected_option = option_count - 1;
          }
          if (IsKeyPressed(KEY_DOWN)) {
              selected_option++;
              if (selected_option >= option_count) selected_option = 0;
          }

          if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
              if (selected_option == 0) {
                  current_screen = MUSIC_MENU;
              } else if (selected_option == 1) {
                  CloseWindow();
                  return 0;
              }
          }

          int title_font = 64;
          int option_font = 40;

          Vector2 title_size = MeasureTextEx(GetFontDefault(), title, title_font, 2);
          DrawText(
            title,
            (WNDW_WIDTH - title_size.x) / 2,
            WNDW_HEIGHT * 0.20f,
            title_font,
            TEXT_COLOR
          );

          float menu_start_y = WNDW_HEIGHT * 0.45f;
          float spacing = 70;

          for (int i = 0; i < option_count; i++) {
            Color col = (i == selected_option) ? SELECT_OPTION_COLOR : TEXT_COLOR;

            Vector2 text_size = MeasureTextEx(GetFontDefault(), options[i], option_font, 2);

            DrawText(
              options[i],
              (WNDW_WIDTH - text_size.x) / 2,
              menu_start_y + i * spacing,
              option_font,
              col
            );
          }
        }
        break;
      case MUSIC_MENU:
        {
          static bool loaded = false;

          if (!loaded) {
            loaded = true;
            #ifdef _WIN32
             WIN32_FIND_DATAA findData;
              HANDLE hFind = FindFirstFileA("assets\\*", &findData);

              if (hFind == INVALID_HANDLE_VALUE) {
                  printf("Could not open assets folder\n");
                  return 1;
              }

              do {
                  if (strcmp(findData.cFileName, ".") == 0 ||
                      strcmp(findData.cFileName, "..") == 0) continue;

                  char fullpath[512];
                  snprintf(fullpath, sizeof(fullpath), "assets/%s", findData.cFileName);

                  struct stat st;
                  if (stat(fullpath, &st) == 0 && S_ISREG(st.st_mode)) {
                      char *copy = _strdup(findData.cFileName);
                      nob_da_append(&music_files, copy);
                  }
              }
              while (FindNextFileA(hFind, &findData));

              FindClose(hFind);
            #else
            DIR *dir = opendir("assets");
            struct dirent *entry;

            if (dir) {
              while ((entry = readdir(dir)) != NULL) {
                  if (strcmp(entry->d_name, ".") == 0 ||
                    strcmp(entry->d_name, "..") == 0) continue;

                  char fullpath[512];
                  snprintf(fullpath, sizeof(fullpath), "assets/%s", entry->d_name);

                  struct stat st;
                  if (stat(fullpath, &st) == 0 && S_ISREG(st.st_mode)) {
                    char *copy = strdup(entry->d_name);
                    nob_da_append(&music_files, copy);
                  }
              }
              closedir(dir);
            }
            #endif
          }

          ClearBackground(BG_COLOR);

          if (music_files.count == 0) {
            DrawText("No music files found in assets directory", 40, 40, 30, TEXT_COLOR);
            break;
          }

          if (IsKeyPressed(KEY_DOWN)) {
            selected_option = (selected_option + 1) % music_files.count;

            if (selected_option >= list_offset + 10)
              list_offset = (list_offset + 1) % music_files.count;
          }

          if (IsKeyPressed(KEY_UP)) {
            selected_option--;
            if (selected_option < 0)
              selected_option = music_files.count - 1;

            if (selected_option < list_offset)
              list_offset--;
            if (list_offset < 0)
              list_offset = music_files.count - 1;
          }

          DrawText("SELECT MUSIC", WNDW_WIDTH / 2 - 200, 40, 48, TEXT_COLOR);

          int option_font = 30;
          int start_y = 150;

          for (int i = 0; i < 10 && (size_t)i < music_files.count; i++) {
            int idx = (list_offset + i) % music_files.count;

            Color col = (idx == selected_option) ? SELECT_OPTION_COLOR : TEXT_COLOR;

            DrawText(
                music_files.items[idx],
                100,
                start_y + i * 45,
                option_font,
                col
            );
          }

          if (IsKeyPressed(KEY_ENTER)) {
            current_screen = PLAYING;
            game_init(music_files.items[selected_option]);
          }

          if (IsKeyPressed(KEY_ESCAPE)) {
              selected_option = 0;
              list_offset = 0;
              current_screen = MENU;
          }
        }
        break;
      case PLAYING:
        {
          if (!game_update()) {
            current_screen = MENU;
            game_close();
          }
        }
        break;
    }
    EndDrawing();
  }
  CloseWindow();
}
