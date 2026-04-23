#pragma once

#include <glad/glad.h>

enum class GBufferTexture { POSITION=0, NORMAL=1, ALBEDO_SPEC=2, EMISSION=3, DEPTH=4 };

class GBuffer {
public:
    GBuffer(int width, int height);
    ~GBuffer();
    
    void Bind() const;
    void Unbind() const;
    void BindTextures() const;
    GLuint GetTexture(GBufferTexture t) const;
    void Resize(int newWidth, int newHeight);
    void BlitDepthTo(GLuint targetFBO, int w, int h) const;
    bool IsComplete() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    GLuint fbo;
    GLuint textures[4];
    GLuint depthRBO;
    int width, height;
    
    void Create();
    void Destroy();
};
