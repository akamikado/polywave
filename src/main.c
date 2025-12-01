#include <dirent.h>
#include <stdlib.h>
#include <string.h>

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

            DIR *dir = opendir("assets");
            struct dirent *entry;

            if (dir) {
              while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) {
                  char *copy = strdup(entry->d_name);
                  nob_da_append(&music_files, copy);
                }
              }
              closedir(dir);
            }
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
