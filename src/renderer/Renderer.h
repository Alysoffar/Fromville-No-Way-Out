#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "GBuffer.h"
#include "PostFXState.h"
#include "ShadowMap.h"
#include "Shader.h"

class DayNightCycle;
class Camera;

struct RenderCommand {
    // Forward declared for future use
};

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    bool Init();

    void BeginGeometryPass();
    void EndGeometryPass();
    void ShadowPass(std::vector<struct RenderCommand>& cmds, glm::vec3 lightDir);
    void SSAOPass();
    void LightingPass(DayNightCycle& dayNight, Camera& camera);
    void PostProcessPass(struct PostFXState state);
    void DrawOverlay(GLuint textureID);
    void Resize(int w, int h);
    float GetAspectRatio() const { return height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height); }

    GBuffer* GetGBuffer() { return gBuffer.get(); }
    ShadowMap* GetShadowMap() { return shadowMap.get(); }

private:
    int width, height;
    std::unique_ptr<GBuffer> gBuffer;
    std::unique_ptr<ShadowMap> shadowMap;

    // Fullscreen quad
    GLuint quadVAO, quadVBO;
    void SetupQuad();
    void DrawQuad();

    // SSAO resources
    GLuint ssaoFBO, ssaoBlurFBO;
    GLuint ssaoColorBuffer, ssaoBlurBuffer;
    GLuint noiseTexture;
    std::vector<glm::vec3> ssaoKernel;
    void InitSSAO();

    // HDR framebuffer (lighting output)
    GLuint hdrFBO, hdrColorBuffer, hdrDepthRBO;
    void InitHDR();

    // Shaders
    std::unique_ptr<Shader> ssaoShader, ssaoBlurShader;
    std::unique_ptr<Shader> lightingShader;
    std::unique_ptr<Shader> postShader;
    std::unique_ptr<Shader> uiShader;
};
