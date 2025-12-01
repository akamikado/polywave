#include <raylib.h>

#include "game.h"
#include "config.h"

#define SELECT_OPTION_COLOR GetColor(0xef946cff)

typedef enum {
  MENU,
  PLAYING
} Screen;

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Screen current_screen = MENU;

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
                  game_init();
                  current_screen = PLAYING;
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
