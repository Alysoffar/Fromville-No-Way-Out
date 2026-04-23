#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    // Directional shadow (moonlight)
    void InitDirectional(int resolution = 4096);
    void BindDirectional();
    void UnbindDirectional();
    GLuint GetDirectionalTexture() const;
    glm::mat4 GetLightSpaceMatrix(glm::vec3 lightDir, glm::vec3 sceneCenter, float sceneRadius);

    // Point light cubemap shadow
    void InitPointLight(int index, int resolution = 512);
    void BindPointLight(int index);
    void UnbindPointLight(int index);
    GLuint GetPointTexture(int index) const;
    std::vector<glm::mat4> GetPointLightMatrices(glm::vec3 lightPos, float nearPlane = 0.1f, float farPlane = 50.0f);

private:
    GLuint dirFBO, dirDepthTexture;
    int dirResolution;
    
    GLuint pointFBOs[8];
    GLuint pointCubemaps[8];
    int pointResolution;
};
