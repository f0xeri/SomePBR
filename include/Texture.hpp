//
// Created by Yaroslav on 29.07.2020.
//

#ifndef OPENGLTEST_TEXTURE_HPP
#define OPENGLTEST_TEXTURE_HPP


#include <string>
#include <GL/glew.h>
#include <vector>

class Texture
{
private:
    const char *name;
    unsigned char *data;
    float *fdata;
    int width, height, nrChannels;
public:
    GLuint texture;
    Texture(const char *name);
    ~Texture();
    void loadTexture();
    void bind();

    unsigned int loadCubemap(std::vector<std::string> faces);
    unsigned int loadHDRmap();

    GLuint loadDDS(const char *path);
};


#endif //OPENGLTEST_TEXTURE_HPP
