#ifndef GAME_H
#define GAME_H

#include <math.h>
#include <iostream>
#include <vector>
#include "raylib.h"
#include "raymath.h"
#include "config.h"

struct Light {
  Vector2 position;
  Vector3 color;
  float   size; // in pixels
};

class Game {
  public:
    Game();
    void update();
    void render();
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();

  private:
    Shader lightingShader;
    int smoothShadows;
    int cascadeAmount;

    bool debug;
    float time;
    double timeSinceModeSwitch;

    std::vector<Light> lights;

    enum Mode {
      DRAWING,
      LIGHTING,
      VIEWING
    } mode;

    struct {
      Image     img;
      Texture2D tex;
    } cursor;

    struct {
      Image     img;
      Texture2D tex;
      float     scale;
    } brush;

    struct {
      Image img;
      Texture2D tex;
    } canvas;
};

#endif /* GAME_H */
