#pragma once

#include <glad/glad.h>

#include "PostFXState.h"

class Shader;

class PostProcess {
public:
	PostProcess(int width, int height);
	~PostProcess();
	void Init();
	void Process(GLuint hdrInput, PostFXState state, float time);
	void Resize(int w, int h);

private:
	int width;
	int height;
	GLuint pingPongFBO[2];
	GLuint pingPongBuffers[2];
	GLuint brightPassFBO;
	GLuint brightPassBuffer;
	GLuint outputFBO;
	GLuint outputBuffer;
	GLuint quadVAO;
	GLuint quadVBO;

	Shader* bloomBrightPass;
	Shader* bloomBlur;
	Shader* bloomComposite;
	Shader* postShader;

	void InitFramebuffers();
	void BloomPass(GLuint hdrInput);
	void DrawQuad();
};

