//
// Created by Yaroslav on 25.02.2021.
//

#ifndef LAB4B_BASEOBJECT_HPP
#define LAB4B_BASEOBJECT_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include "Texture.hpp"

enum ObjectType
{
    BOX,
    SPHERE,
    AREALIGHT,
};

struct Material
{
    glm::vec3 emmitance;
    glm::vec3 reflectance;
    float roughness;
    float opacity;
    float metalness;
    glm::vec3 color;
};

struct MaterialTextures
{
    Texture *albedo = nullptr;
    Texture *normal = nullptr;
    Texture *metallic = nullptr;
    Texture *roughness = nullptr;
    Texture *height = nullptr;
    Texture *ao = nullptr;
};

struct Box
{
    Material material;
    glm::vec3 halfSize;
    glm::mat4 rotation;
    glm::vec3 position;
};

struct VertexVectorStruct
{
public:
    std::vector<glm::fvec3> vertexPositions;
    std::vector<glm::fvec2> vertexTexCoords;
    std::vector<glm::fvec3> vertexNormals;
};

class IObject
{
public:
    IObject(glm::vec3 position) : position(position), startPosition(position), model(glm::mat4(1.0f)) {};
    glm::vec3 position;
    glm::vec3 startPosition;
    glm::mat4 model;
    Texture *texture;
    Material material;
    MaterialTextures materialTextures{};
    glm::vec3 size;
    glm::vec3 center;
    int texScaleX = 1;
    int texScaleY = 1;
    VertexVectorStruct objectData;
    GLuint VAO{};

    virtual void generateVAO() {};
    virtual void update(float dt) {};
    virtual void update(float dt, bool col, float y) {}
    virtual void applyTranslations() {};
    virtual void draw() {};

    bool physicsEnabled = false;
    bool collisionEnabled = false;

    std::vector<GLuint> mBuffers;
    unsigned mIndicesBuffer = 0;
    size_t mIndicesCount = 0;
};

#endif //LAB4B_BASEOBJECT_HPP
