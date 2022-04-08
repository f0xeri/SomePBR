//
// Created by Yaroslav on 13.02.2021.
//

#ifndef MINE_STATE_HPP
#define MINE_STATE_HPP

#include "Camera.h"
#include "Scene.hpp"

struct State
{
    float deltaTime;
    float speed = 12.f;
    GLFWwindow *window;
    Camera *camera;
    Scene *scene;
    float deltaX = 0.0f;
    float deltaY = 0.0f;

    float x = 0.0f;
    float y = 0.0f;
    unsigned int idBuffer;
    unsigned int irradianceMap;
    unsigned int pickedObject = -1;
    bool dragEnabled = false;
    bool cursor_locked = true;
    bool cursor_started = false;
    bool showDebug = true;

    float camX = 0.0f;
    float camY = 0.0f;
    bool vsync = false;

    int nbFrames = 0;
    bool showPolygons = false;

    double lastTime;
};


#endif //MINE_STATE_HPP
