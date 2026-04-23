#include "ShadowMap.h"

ShadowMap::ShadowMap() : dirFBO(0), dirDepthTexture(0), dirResolution(4096), pointResolution(512) {
    for (int i = 0; i < 8; ++i) {
        pointFBOs[i] = 0;
        pointCubemaps[i] = 0;
    }
}

ShadowMap::~ShadowMap() {
    if (dirFBO) glDeleteFramebuffers(1, &dirFBO);
    if (dirDepthTexture) glDeleteTextures(1, &dirDepthTexture);
    
    for (int i = 0; i < 8; ++i) {
        if (pointFBOs[i]) glDeleteFramebuffers(1, &pointFBOs[i]);
        if (pointCubemaps[i]) glDeleteTextures(1, &pointCubemaps[i]);
    }
}

void ShadowMap::InitDirectional(int resolution) {
    dirResolution = resolution;
    
    glGenFramebuffers(1, &dirFBO);
    glGenTextures(1, &dirDepthTexture);
    
    glBindTexture(GL_TEXTURE_2D, dirDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, dirResolution, dirResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, dirFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dirDepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::BindDirectional() {
    glViewport(0, 0, dirResolution, dirResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, dirFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::UnbindDirectional() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint ShadowMap::GetDirectionalTexture() const {
    return dirDepthTexture;
}

glm::mat4 ShadowMap::GetLightSpaceMatrix(glm::vec3 lightDir, glm::vec3 sceneCenter, float sceneRadius) {
    glm::vec3 lightPos = sceneCenter - glm::normalize(lightDir) * 80.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 1.0f, 150.0f);
    return lightProjection * lightView;
}

void ShadowMap::InitPointLight(int index, int resolution) {
    if (index < 0 || index >= 8) return;
    
    pointResolution = resolution;
    
    glGenFramebuffers(1, &pointFBOs[index]);
    glGenTextures(1, &pointCubemaps[index]);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointCubemaps[index]);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, 
                     pointResolution, pointResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, pointFBOs[index]);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointCubemaps[index], 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::BindPointLight(int index) {
    if (index < 0 || index >= 8) return;
    glViewport(0, 0, pointResolution, pointResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, pointFBOs[index]);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::UnbindPointLight(int index) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint ShadowMap::GetPointTexture(int index) const {
    if (index < 0 || index >= 8) return 0;
    return pointCubemaps[index];
}

std::vector<glm::mat4> ShadowMap::GetPointLightMatrices(glm::vec3 lightPos, float nearPlane, float farPlane) {
    std::vector<glm::mat4> shadowTransforms;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    return shadowTransforms;
}
