#include "PostProcess.h"

#include <stdexcept>

#include <glm/glm.hpp>

#include "Shader.h"

PostProcess::PostProcess(int w, int h)
	: width(w), height(h),
	  brightPassFBO(0), brightPassBuffer(0),
	  outputFBO(0), outputBuffer(0),
	  quadVAO(0), quadVBO(0),
	  bloomBrightPass(nullptr), bloomBlur(nullptr), bloomComposite(nullptr), postShader(nullptr) {
	pingPongFBO[0] = pingPongFBO[1] = 0;
	pingPongBuffers[0] = pingPongBuffers[1] = 0;
}

PostProcess::~PostProcess() {
	if (quadVAO) {
		glDeleteVertexArrays(1, &quadVAO);
	}
	if (quadVBO) {
		glDeleteBuffers(1, &quadVBO);
	}

	if (pingPongFBO[0]) {
		glDeleteFramebuffers(2, pingPongFBO);
	}
	if (pingPongBuffers[0]) {
		glDeleteTextures(2, pingPongBuffers);
	}
	if (brightPassFBO) {
		glDeleteFramebuffers(1, &brightPassFBO);
	}
	if (brightPassBuffer) {
		glDeleteTextures(1, &brightPassBuffer);
	}
	if (outputFBO) {
		glDeleteFramebuffers(1, &outputFBO);
	}
	if (outputBuffer) {
		glDeleteTextures(1, &outputBuffer);
	}

	delete bloomBrightPass;
	delete bloomBlur;
	delete bloomComposite;
	delete postShader;
}

void PostProcess::Init() {
	bloomBrightPass = new Shader("Bloom Bright Pass");
	bloomBrightPass->Load("assets/shaders/ssao.vert", "assets/shaders/bloom_bright.frag");
	bloomBrightPass->Bind();
	bloomBrightPass->SetInt("hdrBuffer", 0);
	bloomBrightPass->Unbind();

	bloomBlur = new Shader("Bloom Blur");
	bloomBlur->Load("assets/shaders/ssao.vert", "assets/shaders/bloom_blur.frag");
	bloomBlur->Bind();
	bloomBlur->SetInt("image", 0);
	bloomBlur->Unbind();

	bloomComposite = new Shader("Bloom Composite");
	bloomComposite->Load("assets/shaders/ssao.vert", "assets/shaders/bloom_blur.frag");

	postShader = new Shader("Post Process");
	postShader->Load("assets/shaders/ssao.vert", "assets/shaders/post_process.frag");
	postShader->Bind();
	postShader->SetInt("hdrBuffer", 0);
	postShader->SetInt("bloomBlur", 1);
	postShader->SetInt("ssaoBuffer", 2);
	postShader->Unbind();

	const float quadVertices[] = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);

	InitFramebuffers();
}

void PostProcess::Process(GLuint hdrInput, PostFXState state, float time) {
	if (!postShader || !bloomBrightPass || !bloomBlur) {
		return;
	}

	BloomPass(hdrInput);

	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	postShader->Bind();
	postShader->SetFloat("exposure", 1.0f);
	postShader->SetFloat("desaturation", state.desaturation);
	postShader->SetFloat("vignetteStrength", state.vignetteStrength);
	postShader->SetFloat("screenShakeIntensity", state.screenShakeIntensity);
	postShader->SetFloat("redChromaticAberration", state.redChromaticAberration);
	postShader->SetFloat("symbolSightStrength", state.symbolSightStrength);
	postShader->SetFloat("uvRippleStrength", state.uvRippleStrength);
	postShader->SetVec3("colorGrade", state.colorGrade);
	postShader->SetVec3("fogColorGrade", glm::vec3(1.0f));
	postShader->SetFloat("time", time);
	postShader->SetVec2("resolution", glm::vec2(static_cast<float>(width), static_cast<float>(height)));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrInput);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pingPongBuffers[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);

	DrawQuad();
	postShader->Unbind();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, outputFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::Resize(int w, int h) {
	width = w;
	height = h;
	InitFramebuffers();
}

void PostProcess::InitFramebuffers() {
	if (pingPongFBO[0]) {
		glDeleteFramebuffers(2, pingPongFBO);
		pingPongFBO[0] = pingPongFBO[1] = 0;
	}
	if (pingPongBuffers[0]) {
		glDeleteTextures(2, pingPongBuffers);
		pingPongBuffers[0] = pingPongBuffers[1] = 0;
	}
	if (brightPassFBO) {
		glDeleteFramebuffers(1, &brightPassFBO);
		brightPassFBO = 0;
	}
	if (brightPassBuffer) {
		glDeleteTextures(1, &brightPassBuffer);
		brightPassBuffer = 0;
	}
	if (outputFBO) {
		glDeleteFramebuffers(1, &outputFBO);
		outputFBO = 0;
	}
	if (outputBuffer) {
		glDeleteTextures(1, &outputBuffer);
		outputBuffer = 0;
	}

	glGenFramebuffers(1, &brightPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, brightPassFBO);
	glGenTextures(1, &brightPassBuffer);
	glBindTexture(GL_TEXTURE_2D, brightPassBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightPassBuffer, 0);

	glGenFramebuffers(2, pingPongFBO);
	glGenTextures(2, pingPongBuffers);
	for (int i = 0; i < 2; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingPongBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongBuffers[i], 0);
	}

	glGenFramebuffers(1, &outputFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
	glGenTextures(1, &outputBuffer);
	glBindTexture(GL_TEXTURE_2D, outputBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputBuffer, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::BloomPass(GLuint hdrInput) {
	glBindFramebuffer(GL_FRAMEBUFFER, brightPassFBO);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	bloomBrightPass->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrInput);
	DrawQuad();
	bloomBrightPass->Unbind();

	bool horizontal = true;
	bool firstIter = true;
	constexpr int blurPasses = 6;
	for (int i = 0; i < blurPasses; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[horizontal ? 1 : 0]);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		bloomBlur->Bind();
		bloomBlur->SetBool("horizontal", horizontal);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, firstIter ? brightPassBuffer : pingPongBuffers[horizontal ? 0 : 1]);
		DrawQuad();
		bloomBlur->Unbind();

		horizontal = !horizontal;
		if (firstIter) {
			firstIter = false;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::DrawQuad() {
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

