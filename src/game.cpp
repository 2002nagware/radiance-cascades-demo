#include "game.h"

#define RELOAD_CANVAS() UnloadTexture(canvas.tex); \
                        canvas.tex = LoadTextureFromImage(canvas.img);

//               number of lights, distance from centre
void placeLights(std::vector<Light>* lights, int N = 4, float d = 256.0) {
  for (float i = 0; i < N; i++) {
    float t = (i+1) * (PI*2/N) + 0.1;
    Light l;
    l.position = Vector2{ SCREEN_WIDTH/2 + std::sin(t) * d, SCREEN_HEIGHT/2 + std::cos(t) * d};
    l.color    = Vector3{ std::sin(t), std::cos(t), 1.0 };
    l.size     = 600;
    lights->push_back(l);
  }
}

Game::Game() {
  debug         = false;
  smoothShadows = true;
  mode          = DRAWING;
  cascadeAmount = 512;
  #if __APPLE__
  cascadeAmount = 128;
  #endif

  lightingShader = LoadShader(0, "res/shaders/lighting.frag");
  if (!IsShaderValid(lightingShader)) {
    printf("lightingShader is broken!!\n");
    UnloadShader(lightingShader);
    lightingShader = LoadShader(0, "res/shaders/broken.frag");
  }

  brush.img = LoadImage("res/brush.png");
  brush.tex = LoadTextureFromImage(brush.img);
  brush.scale = 0.25;

  canvas.img = LoadImage("res/canvas.png");
  canvas.tex = LoadTextureFromImage(canvas.img);

  placeLights(&lights);
}

void Game::update() {
  time = GetTime();
  SetShaderValue(lightingShader,    GetShaderLocation(lightingShader, "uTime"),    &time, SHADER_UNIFORM_FLOAT);

  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);

  int lightsAmount = lights.size();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uLightsAmount"), &lightsAmount, SHADER_UNIFORM_INT);

  int apple = 0;
  #ifdef __APPLE__
    apple = 1;
  #endif
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uApple"), &apple, SHADER_UNIFORM_INT);

  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uCascadeAmount"), &cascadeAmount, SHADER_UNIFORM_INT);
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uSmoothShadows"), &smoothShadows, SHADER_UNIFORM_INT);
}

void Game::render() {
  ClearBackground(PINK);

  BeginShaderMode(lightingShader);
    // <!> SetShaderValueTexture() has to be called while the shader is enabled
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uOcclusionMask"), canvas.tex);
    DrawTexture(canvas.tex, 0, 0, WHITE);
  EndShaderMode();

  for (int i = 0; i < lights.size(); i++) {
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].position", i)), &lights[i].position, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].color",    i)), &lights[i].color,    SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].size",     i)), &lights[i].size,     SHADER_UNIFORM_FLOAT);
    if (debug) DrawCircleLinesV(lights[i].position, lights[i].size/64, GREEN);
   }

  switch (mode) {
    case DRAWING:
      DrawTextureEx(brush.tex,
                    Vector2{ (float)(GetMouseX() - brush.img.width/2*brush.scale),
                               (float)(GetMouseY() - brush.img.height/2*brush.scale) },
                    0.0,
                    brush.scale,
                    BLACK);
      break;
    case LIGHTING:
      DrawCircleLines(GetMouseX(), GetMouseY(), (brush.scale*1200)/64, ColorFromNormalized(Vector4{ std::sin(time), std::cos(time), 1.0, 1.0 }));
      break;
  }
}

void Game::renderUI() {
  if (debug) {
    DrawText(TextFormat("%i FPS",            GetFPS()),             0, 0,  1, GREEN);
    DrawText(TextFormat("%i lights",         lights.size()),        0, 8,  1, GREEN);
    if      (mode == DRAWING)  DrawText(TextFormat("%f brush scale",    brush.scale),    0, 32, 1, GREEN);
    else if (mode == LIGHTING) DrawText(TextFormat("%f light size", brush.scale * 2400), 0, 32, 1, GREEN);
    DrawText(TextFormat("%i cascades",       cascadeAmount),        0, 40, 1, GREEN);
    if (smoothShadows) DrawText("smoothShadows ON", 0, 48, 1, GREEN);
  }
}

void Game::processKeyboardInput() {
  if (IsKeyPressed(KEY_ONE))   mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   mode = LIGHTING;
  if (IsKeyPressed(KEY_THREE)) mode = VIEWING;

  if (IsKeyPressed(KEY_F3)) debug = !debug;
  if (IsKeyPressed(KEY_S))  (smoothShadows == 0) ? smoothShadows = 1 : smoothShadows = 0;
  if (IsKeyPressed(KEY_C)) {
    switch (mode) {
      case DRAWING:
        printf("Clearing canvas.\n");
        canvas.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        RELOAD_CANVAS();
        break;
      case LIGHTING:
        printf("Clearing lights.\n");
        lights.clear();
        break;
    }
  }
  if (IsKeyPressed(KEY_R)) {
    if (mode == DRAWING) {
      printf("Replacing canvas.\n");
      canvas.img = LoadImage("res/canvas.png");
      RELOAD_CANVAS();
    } else if (mode == LIGHTING) {
      printf("Replacing lights.\n");
      lights.clear();
      placeLights(&lights);
    }
  }

  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    if (IsKeyPressed(KEY_R)) {
      printf("Reloading shaders.\n");
      UnloadShader(lightingShader);
      lightingShader = LoadShader(0, "res/shaders/lighting.frag");
      if (!IsShaderValid(lightingShader)) {
        UnloadShader(lightingShader);
        lightingShader = LoadShader(0, "res/shaders/broken.frag");
      }
    }
  }
}

void Game::processMouseInput() {
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    int amp = 50;
    if (IsKeyDown(KEY_LEFT_SHIFT)) amp = 1;
    cascadeAmount += GetMouseWheelMove() * amp;
    if (cascadeAmount < 1) cascadeAmount = 1;
  } else {
    brush.scale += GetMouseWheelMove() / 100;
    if (brush.scale < 0.1) brush.scale = 0.1;
  }

  if (IsMouseButtonDown(2)) {
    for (int i = 0; i < lights.size(); i++) {
      if (Vector2Length(lights[i].position - Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }) < 16) {
        lights[i].position = Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) };
      }
    }
  }

  switch (mode) {
    case DRAWING:
      if (IsMouseButtonDown(0)) {
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width/2*brush.scale),
                               static_cast<float>(GetMouseY() - brush.img.height/2*brush.scale),
                               static_cast<float>(brush.img.width * brush.scale),
                               static_cast<float>(brush.img.height * brush.scale) },
                  BLACK);
        RELOAD_CANVAS();
      } else if (IsMouseButtonDown(1)) {
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width/2*brush.scale),
                               static_cast<float>(GetMouseY() - brush.img.height/2*brush.scale),
                               static_cast<float>(brush.img.width*brush.scale),
                               static_cast<float>(brush.img.height*brush.scale) },
                  WHITE);
        RELOAD_CANVAS();
      }
      break;
    case LIGHTING:
      if (IsMouseButtonPressed(0)) {
        Light l;
        l.position = Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) };
        l.color    = Vector3{ std::sin(time), std::cos(time), 1.0 };
        l.size     = brush.scale*1200;//std::abs(std::sin(time)) * 500 + 100;
        lights.push_back(l);
      } else if (IsMouseButtonDown(1)) {
        for (int i = 0; i < lights.size(); i++) {
          if (Vector2Length(lights[i].position - Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }) < 16) {
            lights.erase(lights.begin() + i);
          }
        }
      }
    break;
  }
}
