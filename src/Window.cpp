//
// Created by Yaroslav on 30.10.2020.
//

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/tiny_gltf.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tuple>
#include <glm/ext.hpp>
#include <sstream>
#include <GUIRenderer.hpp>
#include "Window.h"
#include "Logger.hpp"
#include "ObjectLoader.h"
#include "Camera.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Controls.h"
#include "Object/Cube.hpp"
#include "Object/Sphere.hpp"
#include "Scene.hpp"

State *state;
Controls *controls;

unsigned int nbFrames = 0;
double lastTime;
double dx, dy;

float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
};

void showFPS(GLFWwindow *pWindow)
{
    double currentTime = glfwGetTime();
    double delta = currentTime - lastTime;
    nbFrames++;
    if (delta >= 1.0)
    {
        double fps = double(nbFrames) / delta;
        std::stringstream ss;
        ss << "FoxEngine " << " [" << fps << " FPS]";
        glfwSetWindowTitle(pWindow, ss.str().c_str());
        nbFrames = 0;
        lastTime = currentTime;
    }
}

Window::Window(const char *title, int width, int height)
{
    if (!glfwInit())
    {
        LOG("[ERROR] Failed to init GLFW!");
        throw std::runtime_error("GLFW init failed.");
    }
    else LOG("[INFO] GLFW inited.");
    glfwWindowHint(GLFW_SAMPLES, 8);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    mainWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (mainWindow)
    {
        LOG("[INFO] Window opened.");
    }
    else
    {
        LOG("[ERROR] Window open failed!");
        throw std::runtime_error("Window init failed.");
    }
    makeContextCurrent();
    glfwSwapInterval(0);
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        LOG("[ERROR] Failed to init GLEW!");
        throw std::runtime_error("GLEW init failed.");
    }
    else LOG("[INFO] GLEW inited.");
    Window::_width = width;
    Window::_height = height;

    state = new State();
    state->window = mainWindow;
    glfwSwapInterval(state->vsync);
    controls = new Controls(state);

    glfwSetWindowSizeCallback(mainWindow, resizeCallback);
    glfwSetCursorPosCallback(mainWindow, cursorCallback);
    glfwSetKeyCallback(mainWindow, keyCallback);
    glfwSetMouseButtonCallback(mainWindow, mouseButtonCallback);
    toggleCursor(mainWindow);
}

Window::~Window()
{
    glfwDestroyWindow(mainWindow);
    LOG("[INFO] Window closed.");
}

GLuint genModelVAO(VertexVector &model)
{
    std::vector<glm::fvec3> vertexPositions = std::get<0>(model);
    std::vector<glm::fvec2> vertexTexCoords = std::get<1>(model);
    std::vector<glm::fvec3> vertexNormals = std::get<2>(model);

    GLuint verticesVBO;
    GLuint textCoordsVBO;
    GLuint normalsVBO;
    GLuint VAO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &verticesVBO);
    glGenBuffers(1, &textCoordsVBO);
    glGenBuffers(1, &normalsVBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexPositions.size() * sizeof(glm::vec3), vertexPositions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, textCoordsVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexTexCoords.size() * sizeof(glm::vec2), vertexTexCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexNormals.size() * sizeof(glm::vec3), vertexNormals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)nullptr);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

GLuint genSkyboxVAO()
{
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);
    return skyboxVAO;
}

inline float squared(float v) { return v * v; }
bool doesCubeIntersectSphere(Cube *cube, Sphere *sphere)
{
    if (!cube->collisionEnabled || !sphere->collisionEnabled) return false;

    float dist_squared = sphere->radius * sphere->radius;
    if (sphere->position.x < cube->Bmin.x) dist_squared -= squared(sphere->position.x - cube->Bmin.x);
    else if (sphere->position.x > cube->Bmax.x) dist_squared -= squared(sphere->position.x - cube->Bmax.x);
    if (sphere->position.y < cube->Bmin.y) dist_squared -= squared(sphere->position.y - cube->Bmin.y);
    else if (sphere->position.y > cube->Bmax.y) dist_squared -= squared(sphere->position.y - cube->Bmax.y);
    if (sphere->position.z < cube->Bmin.z) dist_squared -= squared(sphere->position.z - cube->Bmin.z);
    else if (sphere->position.z > cube->Bmax.z) dist_squared -= squared(sphere->position.z - cube->Bmax.z);
    return dist_squared > 0;
}

void renderDemoScene(Shader &shader, Cube *cube, Cube *cube2, Sphere *sphere)
{
    glActiveTexture(GL_TEXTURE0);
    cube->texture->bind();
    glm::mat4 model = glm::mat4(1.0f);
    shader.uniformMatrix(model, "model");
    if (state->pickedObject == 0)
        shader.setInt(1, "isPicked");
    else
        shader.setInt(0, "isPicked");
    cube->draw();

    glActiveTexture(GL_TEXTURE0);
    cube2->texture->bind();
    model = glm::mat4(1.0f);
    shader.uniformMatrix(model, "model");
    if (state->pickedObject == 1)
        shader.setInt(1, "isPicked");
    else
        shader.setInt(0, "isPicked");
    cube2->draw();

    if (doesCubeIntersectSphere(cube, sphere))
    {
        sphere->update(state->deltaTime, true, cube->position.y);
    }

    if (doesCubeIntersectSphere(cube2, sphere))
    {
        sphere->update(state->deltaTime, true, cube2->position.y);
    }
    else
    {
        sphere->update(state->deltaTime, false, 0.0);
    }

    vec3 sphereTranslate = sphere->position - sphere->startPosition;
    model = glm::mat4(1.0f);
    model = glm::translate(model, sphereTranslate);
    shader.uniformMatrix(model, "model");
    if (state->pickedObject == 2)
    {
        // с этим условием получаются рывки, если пытаться сделать норм скорость
        // без - объект продолжает двигаться даже если не двигать мышкой, ибо сохраняется последние данные deltaX и deltaY
        if (state->x - dx != 0 || state->y - dy != 0)
        {
            sphere->position.x += state->deltaX * state->deltaTime;
            sphere->position.z += state->deltaY * state->deltaTime;

            sphereTranslate = sphere->position - sphere->startPosition;
            model = glm::mat4(1.0f);
            model = glm::translate(model, sphereTranslate);
            shader.uniformMatrix(model, "model");
        }
        shader.setInt(1, "isPicked");
    }
    else
        shader.setInt(0, "isPicked");
    sphere->draw();
}

void renderScene(Shader &shader, Scene *scene)
{
    for (size_t i = 0; i < scene->objects.size(); i++)
    {
        scene->objects[i]->update(state->deltaTime, false, 0.0);
        shader.uniformMatrix(scene->objects[i]->model, "model");
        if (state->pickedObject == i)
            shader.setInt(1, "isPicked");
        else
            shader.setInt(0, "isPicked");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, scene->objects[i]->materialTextures.albedo->texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, scene->objects[i]->materialTextures.normal->texture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, scene->objects[i]->materialTextures.metallic->texture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, scene->objects[i]->materialTextures.roughness->texture);
        glActiveTexture(GL_TEXTURE4);
        if (scene->objects[i]->materialTextures.height != nullptr) glBindTexture(GL_TEXTURE_2D, scene->objects[i]->materialTextures.height->texture);
        glActiveTexture(GL_TEXTURE5);
        if (scene->objects[i]->materialTextures.ao != nullptr) glBindTexture(GL_TEXTURE_2D, scene->objects[i]->materialTextures.ao->texture);
        glActiveTexture(GL_TEXTURE0);
        /*glUniform1f(glGetUniformLocation(shader.mProgram, "roughness"), scene->objects[i]->material.roughness);
        glUniform1f(glGetUniformLocation(shader.mProgram, "metalness"), scene->objects[i]->material.metalness);*/
        glUniform3f(glGetUniformLocation(shader.mProgram, "emmisivity"), scene->objects[i]->material.emmitance.x, scene->objects[i]->material.emmitance.y, scene->objects[i]->material.emmitance.z);
        scene->objects[i]->draw();
    }
}

void renderSceneId(Shader &shader, Scene *scene)
{
    for (int i = 0; i < scene->objects.size(); i++)
    {
        /*vec3 objPosTranslate = scene->objects[i]->position - scene->objects[i]->startPosition;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, objPosTranslate);*/
        shader.uniformMatrix(scene->objects[i]->model, "model");
        int r = (i & 0x000000FF) >>  0;
        int g = (i & 0x0000FF00) >>  8;
        int b = (i & 0x00FF0000) >> 16;
        glUniform4f(glGetUniformLocation(shader.mProgram, "color"), r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        scene->objects[i]->draw();
    }
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

long double operator "" _mm(long double mm)
{
    return mm / 320.0f;
}

long double operator "" _mm(unsigned long long mm)
{
    return mm / 320.0f;
}


void Window::startLoop()
{
    glEnable(GL_DEPTH_TEST);
    ObjectLoader loader;
    GUIRenderer gui(mainWindow);

    Texture *texture = new Texture("res/textures/cube.jpg");

    Texture *floorTexture = new Texture("res/textures/floor.jpg");
    floorTexture->loadTexture();

    Texture *woodTexture = new Texture("res/textures/woodTexture.jpg");
    woodTexture->loadTexture();

    Texture *skyboxTexture = new Texture("skybox");
    std::vector<std::string> faces
            {
                    "res/textures/right.jpg",
                    "res/textures/left.jpg",
                    "res/textures/top.jpg",
                    "res/textures/bottom.jpg",
                    "res/textures/front.jpg",
                    "res/textures/back.jpg"
            };

    Texture *rustedIron2Albedo = new Texture("res/textures/rustediron2/rustediron2_basecolor.png");
    rustedIron2Albedo->loadTexture();
    Texture *rustedIron2Normal = new Texture("res/textures/rustediron2/rustediron2_normal.png");
    rustedIron2Normal->loadTexture();
    Texture *rustedIron2Metallic = new Texture("res/textures/rustediron2/rustediron2_metallic.png");
    rustedIron2Metallic->loadTexture();
    Texture *rustedIron2Roughness = new Texture("res/textures/rustediron2/rustediron2_roughness.png");
    rustedIron2Roughness->loadTexture();

    Texture *goldAlbedo = new Texture("res/textures/gold/albedo.png");
    goldAlbedo->loadTexture();
    Texture *goldNormal = new Texture("res/textures/gold/normal.png");
    goldNormal->loadTexture();
    Texture *goldMetallic = new Texture("res/textures/gold/metallic.png");
    goldMetallic->loadTexture();
    Texture *goldRoughness = new Texture("res/textures/gold/roughness.png");
    goldRoughness->loadTexture();

    /*Texture *rockAlbedo = new Texture("res/textures/rock/eroded-smoothed-rockface_albedo.png");
    rockAlbedo->loadTexture();
    Texture *rockNormal = new Texture("res/textures/rock/eroded-smoothed-rockface_normal-ogl.png");
    rockNormal->loadTexture();
    Texture *rockMetallic = new Texture("res/textures/rock/eroded-smoothed-rockface_metallic.png");
    rockMetallic->loadTexture();
    Texture *rockRoughness = new Texture("res/textures/rock/eroded-smoothed-rockface_roughness.png");
    rockRoughness->loadTexture();
    Texture *rockHeight = new Texture("res/textures/rock/eroded-smoothed-rockface_height.png");
    rockHeight->loadTexture();
    Texture *rockAO = new Texture("res/textures/rock/eroded-smoothed-rockface_ao.png");
    rockAO->loadTexture();*/

    Texture *graniteAlbedo = new Texture("res/textures/granite/gray-granite-flecks-albedo.png");
    graniteAlbedo->loadTexture();
    Texture *graniteNormal = new Texture("res/textures/granite/gray-granite-flecks-Normal-ogl.png");
    graniteNormal->loadTexture();
    Texture *graniteMetallic = new Texture("res/textures/granite/gray-granite-flecks-Metallic.png");
    graniteMetallic->loadTexture();
    Texture *graniteRoughness = new Texture("res/textures/granite/gray-granite-flecks-Roughness.png");
    graniteRoughness->loadTexture();
    Texture *graniteAO = new Texture("res/textures/granite/gray-granite-flecks-ao.png");
    graniteAO->loadTexture();

    Texture *rubberAlbedo = new Texture("res/textures/rubber/synth-rubber-albedo.png");
    rubberAlbedo->loadTexture();
    Texture *rubberNormal = new Texture("res/textures/rubber/synth-rubber-normal.png");
    rubberNormal->loadTexture();
    Texture *rubberMetallic = new Texture("res/textures/rubber/synth-rubber-metalness.png");
    rubberMetallic->loadTexture();
    Texture *rubberRoughness = new Texture("res/textures/rubber/synth-rubber-roughness.png");
    rubberRoughness->loadTexture();

    Texture *iceFieldAlbedo = new Texture("res/textures/iceField/ice_field_albedo.png");
    iceFieldAlbedo->loadTexture();
    Texture *iceFieldNormal = new Texture("res/textures/iceField/ice_field_normal-ogl.png");
    iceFieldNormal->loadTexture();
    Texture *iceFieldMetallic = new Texture("res/textures/iceField/ice_field_metallic.png");
    iceFieldMetallic->loadTexture();
    Texture *iceFieldRoughness = new Texture("res/textures/iceField/ice_field_roughness.png");
    iceFieldRoughness->loadTexture();
    Texture *iceFieldHeight = new Texture("res/textures/iceField/ice_field_height.png");
    iceFieldHeight->loadTexture();
    Texture *iceFieldAO = new Texture("res/textures/iceField/ice_field_ao.png");
    iceFieldAO->loadTexture();

    Texture *hdrMap = new Texture("res/textures/felsenlabyrinth_1k.hdr");
    hdrMap->loadHDRmap();


    Scene *scene = new Scene();
    /*for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            glm::vec3 startPos{(j - (7 / 2)) * 2.5, (i - (7 / 2)) * 2.5, 10};
            Sphere *sphere = new Sphere(startPos, 1.0f);
            sphere->physicsEnabled = false;
            sphere->collisionEnabled = false;
            sphere->texture = rustedIron2Albedo;
            sphere->material.roughness = glm::clamp((float)j / 7.0f, 0.05f, 1.0f);
            sphere->material.metalness = (float)i / 7.0f;
            sphere->material.opacity = 0.0;
            sphere->material.emmitance = vec3(0.0);
            sphere->material.reflectance = vec3(1.0, 1.0, 1.0);
            sphere->material.color = vec3(1, 1, 1);

            sphere->materialTextures.albedo = rustedIron2Albedo;
            sphere->materialTextures.normal = rustedIron2Normal;
            sphere->materialTextures.metallic = rustedIron2Metallic;
            sphere->materialTextures.roughness = rustedIron2Roughness;

            sphere->model = mat4(1.0f);
            sphere->model = glm::translate(sphere->model, sphere->position);
            sphere->model = glm::rotate(sphere->model, glm::radians(90.0f), {1, 0, 0});
            sphere->model = glm::translate(sphere->model, -sphere->position);
            sphere->generateVAO();
            scene->addObject(sphere);
        }
    }*/

    glm::vec3 startPos1{0, 0, 0};
    Sphere *sphere1 = new Sphere(startPos1, 1.0f);
    sphere1->physicsEnabled = false;
    sphere1->collisionEnabled = false;

    sphere1->material.roughness = glm::clamp((float)1 / 7.0f, 0.05f, 1.0f);
    sphere1->material.metalness = (float)1 / 7.0f;
    sphere1->material.opacity = 0.0;
    sphere1->material.emmitance = vec3(0.0);
    sphere1->material.reflectance = vec3(1.0, 1.0, 1.0);
    sphere1->material.color = vec3(1, 1, 1);

    sphere1->materialTextures.albedo = rustedIron2Albedo;
    sphere1->materialTextures.normal = rustedIron2Normal;
    sphere1->materialTextures.metallic = rustedIron2Metallic;
    sphere1->materialTextures.roughness = rustedIron2Roughness;
    sphere1->model = mat4(1.0f);
    sphere1->model = glm::translate(sphere1->model, sphere1->position);
    sphere1->model = glm::rotate(sphere1->model, glm::radians(90.0f), {1, 0, 0});
    sphere1->model = glm::translate(sphere1->model, -sphere1->position);
    sphere1->generateVAO();
    scene->addObject(sphere1);

    glm::vec3 startPos2{2, 0, 0};
    Sphere *sphere2 = new Sphere(startPos2, 1.0f);
    sphere2->physicsEnabled = false;
    sphere2->collisionEnabled = false;

    sphere2->material.roughness = glm::clamp((float)2 / 7.0f, 0.05f, 1.0f);
    sphere2->material.metalness = (float)2 / 7.0f;
    sphere2->material.opacity = 0.0;
    sphere2->material.emmitance = vec3(0.0);
    sphere2->material.reflectance = vec3(1.0, 1.0, 1.0);
    sphere2->material.color = vec3(1, 1, 1);

    sphere2->materialTextures.albedo = graniteAlbedo;
    sphere2->materialTextures.normal = graniteNormal;
    sphere2->materialTextures.metallic = graniteMetallic;
    sphere2->materialTextures.roughness = graniteRoughness;
    sphere2->materialTextures.ao = graniteAO;
    sphere2->model = mat4(1.0f);
    sphere2->model = glm::translate(sphere2->model, sphere2->position);
    sphere2->model = glm::rotate(sphere2->model, glm::radians(90.0f), {1, 0, 0});
    sphere2->model = glm::translate(sphere2->model, -sphere2->position);
    sphere2->generateVAO();
    scene->addObject(sphere2);

    glm::vec3 startPos3{4, 0, 0};
    Sphere *sphere3 = new Sphere(startPos3, 1.0f);
    sphere3->physicsEnabled = false;
    sphere3->collisionEnabled = false;

    sphere3->material.roughness = glm::clamp((float)2 / 7.0f, 0.05f, 1.0f);
    sphere3->material.metalness = (float)2 / 7.0f;
    sphere3->material.opacity = 0.0;
    sphere3->material.emmitance = vec3(0.0);
    sphere3->material.reflectance = vec3(1.0, 1.0, 1.0);
    sphere3->material.color = vec3(1, 1, 1);

    sphere3->materialTextures.albedo = rubberAlbedo;
    sphere3->materialTextures.normal = rubberNormal;
    sphere3->materialTextures.metallic = rubberMetallic;
    sphere3->materialTextures.roughness = rubberRoughness;
    sphere3->model = mat4(1.0f);
    sphere3->model = glm::translate(sphere3->model, sphere2->position);
    sphere3->model = glm::rotate(sphere3->model, glm::radians(90.0f), {1, 0, 0});
    sphere3->model = glm::translate(sphere3->model, -sphere2->position);
    sphere3->generateVAO();
    scene->addObject(sphere3);

    glm::vec3 startPos4{6, 0, 0};
    Sphere *sphere4 = new Sphere(startPos4, 1.0f);
    sphere4->physicsEnabled = false;
    sphere4->collisionEnabled = false;

    sphere4->material.roughness = glm::clamp((float)2 / 7.0f, 0.05f, 1.0f);
    sphere4->material.metalness = (float)2 / 7.0f;
    sphere4->material.opacity = 0.0;
    sphere4->material.emmitance = vec3(0.0);
    sphere4->material.reflectance = vec3(1.0, 1.0, 1.0);
    sphere4->material.color = vec3(1, 1, 1);

    sphere4->materialTextures.albedo = iceFieldAlbedo;
    sphere4->materialTextures.normal = iceFieldNormal;
    sphere4->materialTextures.metallic = iceFieldMetallic;
    sphere4->materialTextures.roughness = iceFieldRoughness;
    sphere4->materialTextures.height = iceFieldHeight;
    sphere4->materialTextures.ao = iceFieldAO;
    sphere4->model = mat4(1.0f);
    sphere4->model = glm::translate(sphere4->model, sphere2->position);
    sphere4->model = glm::rotate(sphere4->model, glm::radians(90.0f), {1, 0, 0});
    sphere4->model = glm::translate(sphere4->model, -sphere2->position);
    sphere4->generateVAO();
    scene->addObject(sphere4);

    vec3 lightPos(10, 30, -10);

    auto skybox = genSkyboxVAO();
    state->scene = scene;

    glEnable(GL_DEPTH_TEST);
    state->camera = new Camera(glm::vec3(0, 0, 0), radians(60.0f));
    state->camera->front = {0, 0, 1};

    unsigned int cubemapTexture = skyboxTexture->loadCubemap(faces);
    texture->loadTexture();

    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //unsigned int idBuffer;
    glGenFramebuffers(1, &state->idBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, state->idBuffer);
    unsigned int idColor;
    glGenTextures(1, &idColor);
    glBindTexture(GL_TEXTURE_2D, idColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Window::_width, Window::_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, idColor, 0);
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int resBuffer;
    glGenFramebuffers(1, &resBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, resBuffer);
    unsigned int resBufferColor;
    glGenTextures(1, &resBufferColor);
    glBindTexture(GL_TEXTURE_2D, resBufferColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Window::_width, Window::_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resBufferColor, 0);
    unsigned int attachments2[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments2);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Shader shader("vert", "frag");
    shader.link();

    Shader colorIdShader("colorPickVert", "colorPickFrag");
    colorIdShader.link();

    Shader skyboxShader("vertSkybox", "fragSkybox");
    skyboxShader.link();

    Shader simpleDepthShader("vertDepthShader", "fragDepthShader");
    simpleDepthShader.link();

    Shader debugQuad("vertDebugQuad", "fragDebugQuad");
    debugQuad.setInt(0, "depthMap");
    debugQuad.link();
    vec3 oldLightPos = lightPos;
    double lastTime = glfwGetTime();
    state->camera->front = {0, 0, 1};

    glm::vec3 startPos{lightPos};
    Sphere *sphere = new Sphere(startPos, 1.0f);
    sphere->physicsEnabled = false;
    sphere->collisionEnabled = false;
    sphere->texture = texture;
    sphere->material.roughness = 0.01;
    sphere->material.metalness = 1.0;
    sphere->material.opacity = 0.0;
    sphere->material.emmitance = vec3(1.0);
    sphere->material.reflectance = vec3(1.0, 1.0, 1.0);
    sphere->material.color = vec3(1, 1, 1);

    sphere->materialTextures.albedo = goldAlbedo;
    sphere->materialTextures.normal = goldNormal;
    sphere->materialTextures.metallic = goldMetallic;
    sphere->materialTextures.roughness = goldRoughness;
    sphere->generateVAO();
    scene->addObject(sphere);

    //~~~~~~~~~~~~~~~~~~~~~
    tinygltf::Model model;
    tinygltf::TinyGLTF gltfLoader;
    std::string err;
    std::string warn;

    /*bool ret = gltfLoader.LoadASCIIFromFile(&model, &err, &warn, "res/sponza-gltf/sponza.gltf");
    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
    }
    */
    //~~~~~~~~~~~~~~~~~~~~~
    while (!glfwWindowShouldClose(mainWindow))
    {
        showFPS(mainWindow);
        double currentTime = glfwGetTime();
        state->deltaTime = glfwGetTime() - lastTime;
        lastTime = currentTime;
        glfwGetCursorPos(mainWindow, &dx, &dy);
        //state->deltaX = state->deltaY = 0;
        updateInputs(mainWindow);

        oldLightPos = lightPos;
        float rad = glm::radians(90.0f);
        lightPos.z = (oldLightPos.z * glm::cos(rad * state->deltaTime) - oldLightPos.y * glm::sin(rad * state->deltaTime));
        lightPos.y = (oldLightPos.z * glm::sin(rad * state->deltaTime) + oldLightPos.y * glm::cos(rad * state->deltaTime));
        sphere->position = lightPos;
        sphere->applyTranslations();
        glm::vec3 sphereTranslate = sphere->position - sphere->startPosition;
        sphere->model = glm::mat4(1.0f);
        sphere->model = glm::translate(sphere->model, sphereTranslate);


        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 80.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(-2.0f, 0.0f, -2.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        // render scene from light's point of view
        simpleDepthShader.use();
        simpleDepthShader.uniformMatrix(lightSpaceMatrix, "lightSpaceMatrix");

        glViewport(0, 0,  SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            texture->bind();
        renderScene(simpleDepthShader, scene);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // color id
        glBindFramebuffer(GL_FRAMEBUFFER, state->idBuffer);
        glViewport(0, 0, Window::_width, Window::_height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        colorIdShader.use();
        colorIdShader.uniformMatrix(state->camera->getProjectionMatrix() * state->camera->getViewMatrix(), "projView");
        renderSceneId(colorIdShader, scene);
        //glFlush();
        //glFinish();
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        //glReadBuffer(GL_COLOR_ATTACHMENT0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // reset viewport
        glViewport(0, 0, Window::_width, Window::_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (state->showPolygons) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        shader.use();
        shader.uniformMatrix(state->camera->getProjectionMatrix() * state->camera->getViewMatrix(), "projView");
        glUniform3f(glGetUniformLocation(shader.mProgram, "viewPos"), state->camera->pos.x, state->camera->pos.y, state->camera->pos.z);
        glUniform3f(glGetUniformLocation(shader.mProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        shader.uniformMatrix(lightSpaceMatrix, "lightSpaceMatrix");

        glUniform1i(glGetUniformLocation(shader.mProgram, "uSamples"), 8);
        glUniform1i(glGetUniformLocation(shader.mProgram, "uTime"), glfwGetTime());
        glUniform2f(glGetUniformLocation(shader.mProgram, "uViewportSize"), Window::_width, Window::_height);
        glUniform3f(glGetUniformLocation(shader.mProgram, "uPosition"), state->camera->pos.x, state->camera->pos.y, state->camera->pos.z);
        glUniform3f(glGetUniformLocation(shader.mProgram, "uDirection"), state->camera->front.x, state->camera->front.y, state->camera->front.z);
        glUniform3f(glGetUniformLocation(shader.mProgram, "uUp"), state->camera->up.x, state->camera->up.y, state->camera->up.z);
        glUniform1f(glGetUniformLocation(shader.mProgram, "uFOV"), state->camera->FOV);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        renderScene(shader, scene);

        debugQuad.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        //renderQuad();

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxTexture->bind();
        glm::mat4 model2 = glm::mat4(1.f);
        skyboxShader.uniformMatrix(model2, "view");
        skyboxShader.uniformMatrix(state->camera->getProjectionMatrix() * glm::mat4(glm::mat3(state->camera->getViewMatrix())), "projection");
        glBindVertexArray(skybox);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        gui.render(state);

        glfwSwapBuffers(mainWindow);
        glfwPollEvents();
    }
}

void Window::makeContextCurrent()
{
    glfwMakeContextCurrent(mainWindow);
}
