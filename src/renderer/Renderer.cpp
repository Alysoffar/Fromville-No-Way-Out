#include "Renderer.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <string>

#include "renderer/Camera.h"
#include "world/DayNightCycle.h"

Renderer::Renderer(int width, int height) : width(width), height(height), quadVAO(0), quadVBO(0) {
    gBuffer = std::make_unique<GBuffer>(width, height);
    shadowMap = std::make_unique<ShadowMap>();

    ssaoFBO = 0;
    ssaoBlurFBO = 0;
    ssaoColorBuffer = 0;
    ssaoBlurBuffer = 0;
    noiseTexture = 0;

    hdrFBO = 0;
    hdrColorBuffer = 0;
    hdrDepthRBO = 0;
}

Renderer::~Renderer() {
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);

    if (ssaoFBO) glDeleteFramebuffers(1, &ssaoFBO);
    if (ssaoBlurFBO) glDeleteFramebuffers(1, &ssaoBlurFBO);
    if (ssaoColorBuffer) glDeleteTextures(1, &ssaoColorBuffer);
    if (ssaoBlurBuffer) glDeleteTextures(1, &ssaoBlurBuffer);
    if (noiseTexture) glDeleteTextures(1, &noiseTexture);

    if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
    if (hdrColorBuffer) glDeleteTextures(1, &hdrColorBuffer);
    if (hdrDepthRBO) glDeleteRenderbuffers(1, &hdrDepthRBO);
}

bool Renderer::Init() {
    try {
        shadowMap->InitDirectional(4096);
        for (int i = 0; i < 8; ++i) {
            shadowMap->InitPointLight(i, 512);
        }

        ssaoShader = std::make_unique<Shader>("SSAO");
        ssaoShader->Load("assets/shaders/ssao.vert", "assets/shaders/ssao.frag");
        ssaoShader->Bind();
        ssaoShader->SetInt("gPosition", 0);
        ssaoShader->SetInt("gNormal", 1);
        ssaoShader->SetInt("noiseTexture", 2);
        ssaoShader->Unbind();

        ssaoBlurShader = std::make_unique<Shader>("SSAO Blur");
        ssaoBlurShader->Load("assets/shaders/ssao.vert", "assets/shaders/ssao_blur.frag");
        ssaoBlurShader->Bind();
        ssaoBlurShader->SetInt("ssaoInput", 0);
        ssaoBlurShader->Unbind();

        lightingShader = std::make_unique<Shader>("Deferred Lighting");
        lightingShader->Load("assets/shaders/lighting.vert", "assets/shaders/lighting.frag");
        lightingShader->Bind();
        lightingShader->SetInt("gPosition", 0);
        lightingShader->SetInt("gNormal", 1);
        lightingShader->SetInt("gAlbedoSpec", 2);
        lightingShader->SetInt("gEmission", 3);
        lightingShader->SetInt("ssaoBlur", 4);
        lightingShader->SetInt("shadowMap", 5);
        lightingShader->Unbind();

        // Post shader is optional at this stage and can be loaded later.
    } catch (const std::exception& e) {
        std::cerr << "Renderer init failed: " << e.what() << '\n';
        return false;
    }

    SetupQuad();
    InitSSAO();
    InitHDR();

    if (!gBuffer || !gBuffer->IsComplete()) {
        std::cerr << "Renderer init failed: g-buffer is incomplete.\n";
        return false;
    }

    return true;
}

void Renderer::SetupQuad() {
    float quadVertices[] = {
        // positions        // texCoords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
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

void Renderer::DrawOverlay(GLuint textureID) {
    if (!uiShader) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    uiShader->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    uiShader->SetInt("overlay", 0);
    DrawQuad();
    uiShader->Unbind();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::DrawQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Renderer::InitSSAO() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    ssaoKernel.clear();
    ssaoKernel.reserve(64);

    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) // z >= 0
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator); // scale [0, 1]
        
        // accelerating interpolation
        float scale = (float)i / 64.0f;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO framebuffer is incomplete.\n";
    }

    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoBlurBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO blur framebuffer is incomplete.\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::InitHDR() {
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    
    glGenTextures(1, &hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);

    glGenRenderbuffers(1, &hdrDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "HDR framebuffer is incomplete.\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BeginGeometryPass() {
    if (!gBuffer) {
        return;
    }
    gBuffer->Bind();
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST); // Ensure depth testing is on
    glEnable(GL_CULL_FACE);  // Re-enable culling for geometry
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndGeometryPass() {
    if (!gBuffer) {
        return;
    }
    gBuffer->Unbind();
}

void Renderer::ShadowPass(std::vector<struct RenderCommand>& cmds, glm::vec3 lightDir) {
    (void)cmds;
    (void)lightDir;

    if (!shadowMap) {
        return;
    }

    // Placeholder shadow pass; command rendering hookup will be added later.
    shadowMap->BindDirectional();
    glClear(GL_DEPTH_BUFFER_BIT);
    shadowMap->UnbindDirectional();
}

void Renderer::SSAOPass() {
    if (!ssaoShader || !ssaoBlurShader || !gBuffer) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glViewport(0, 0, width, height);
    glDisable(GL_CULL_FACE); // Fullscreen passes don't need culling
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader->Bind();
    for (int i = 0; i < static_cast<int>(ssaoKernel.size()); ++i) {
        ssaoShader->SetVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }
    ssaoShader->SetMat4("projection", glm::mat4(1.0f));
    ssaoShader->SetMat4("view", glm::mat4(1.0f));
    ssaoShader->SetVec2("noiseScale", glm::vec2(static_cast<float>(width) / 4.0f, static_cast<float>(height) / 4.0f));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetTexture(GBufferTexture::POSITION));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetTexture(GBufferTexture::NORMAL));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);

    DrawQuad();
    ssaoShader->Unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoBlurShader->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    DrawQuad();
    ssaoBlurShader->Unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::LightingPass(DayNightCycle& dayNight, Camera& camera) {
    if (!lightingShader || !gBuffer) {
        return;
    }

    const float timeOfDay = dayNight.GetTimeOfDay();
    const glm::vec3 lightColor = dayNight.GetDirectionalLightColor();
    const glm::vec3 ambientColor = dayNight.GetAmbientColor();
    const glm::vec3 fogColor = dayNight.GetFogColor();
    const float fogDensity = dayNight.GetFogDensity();
    const glm::vec3 lightDir = dayNight.GetSunDirection();

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glViewport(0, 0, width, height);
    glDisable(GL_CULL_FACE); // Fullscreen passes don't need culling
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    lightingShader->Bind();

    lightingShader->SetMat4("lightSpaceMatrix", glm::mat4(1.0f));
    lightingShader->SetVec3("lightDir", lightDir);
    lightingShader->SetVec3("lightColor", lightColor);
    lightingShader->SetVec3("viewPos", camera.position);
    lightingShader->SetVec3("ambientColor", ambientColor);
    lightingShader->SetVec3("fogColor", fogColor);
    lightingShader->SetFloat("fogDensity", fogDensity);
    lightingShader->SetFloat("time", timeOfDay);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetTexture(GBufferTexture::POSITION));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetTexture(GBufferTexture::NORMAL));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetTexture(GBufferTexture::ALBEDO_SPEC));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetTexture(GBufferTexture::EMISSION));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurBuffer);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowMap->GetDirectionalTexture());

    DrawQuad();
    lightingShader->Unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::PostProcessPass(struct PostFXState state) {
    (void)state;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, hdrFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Resize(int w, int h) {
    width = w;
    height = h;

    if (gBuffer) {
        gBuffer->Resize(width, height);
    }

    if (ssaoColorBuffer) {
        glDeleteTextures(1, &ssaoColorBuffer);
        ssaoColorBuffer = 0;
    }
    if (ssaoBlurBuffer) {
        glDeleteTextures(1, &ssaoBlurBuffer);
        ssaoBlurBuffer = 0;
    }
    if (ssaoFBO) {
        glDeleteFramebuffers(1, &ssaoFBO);
        ssaoFBO = 0;
    }
    if (ssaoBlurFBO) {
        glDeleteFramebuffers(1, &ssaoBlurFBO);
        ssaoBlurFBO = 0;
    }
    InitSSAO();

    if (hdrColorBuffer) {
        glDeleteTextures(1, &hdrColorBuffer);
        hdrColorBuffer = 0;
    }
    if (hdrDepthRBO) {
        glDeleteRenderbuffers(1, &hdrDepthRBO);
        hdrDepthRBO = 0;
    }
    if (hdrFBO) {
        glDeleteFramebuffers(1, &hdrFBO);
        hdrFBO = 0;
    }
    InitHDR();
}
