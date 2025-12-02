#include <raylib.h>
#include <math.h>

int main() {
  InitWindow(400, 400, "Icon");
  while(!WindowShouldClose()){
    BeginDrawing();
    ClearBackground(GetColor(0xeff7f6ff));
    DrawCircleV((Vector2){.x = 200, .y = 200}, 200, GetColor(0xef946cff));
    DrawCircleV((Vector2){.x = 200, .y = 200}, 100, GetColor(0xfb232eff));
    EndDrawing();
  }
}
