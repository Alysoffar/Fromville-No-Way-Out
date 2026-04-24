#include "Texture.h"
#include <cstdio>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

GLuint Texture::Load(const char* path) {
    int w, h, c;
    unsigned char* data = stbi_load(path, &w, &h, &c, 0);
    if (!data) {
        std::printf("Failed to load texture: %s\n", path);
        return 0;
    }
    std::printf("Successfully loaded texture: %s (%dx%d, %d channels)\n", path, w, h, c);
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    
    GLenum format = (c == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    return id;
}
