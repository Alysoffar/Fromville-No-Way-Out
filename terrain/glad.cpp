// GLAD OpenGL Loader - Complete loader for GL 3.3 Core functions
// Used by the Fromville terrain renderer

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct gladGLversionStruct GLVersion = { 0, 0 };

// Core GL 1.0-1.1
PFNGLCLEARPROC glad_glClear = nullptr;
PFNGLCLEARCOLORPROC glad_glClearColor = nullptr;
PFNGLENABLEPROC glad_glEnable = nullptr;
PFNGLDISABLEPROC glad_glDisable = nullptr;
PFNGLGETSTRINGPROC glad_glGetString = nullptr;
PFNGLCULLFACEPROC glad_glCullFace = nullptr;
PFNGLFRONTFACEPROC glad_glFrontFace = nullptr;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = nullptr;
PFNGLDEPTHMASKPROC glad_glDepthMask = nullptr;
PFNGLVIEWPORTPROC glad_glViewport = nullptr;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = nullptr;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = nullptr;
PFNGLBLENDFUNCPROC glad_glBlendFunc = nullptr;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = nullptr;
PFNGLPOLYGONMODEPROC glad_glPolygonMode = nullptr;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = nullptr;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = nullptr;

// Textures
PFNGLGENTEXTURESPROC glad_glGenTextures = nullptr;
PFNGLBINDTEXTUREPROC glad_glBindTexture = nullptr;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = nullptr;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = nullptr;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = nullptr;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = nullptr;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = nullptr;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = nullptr;

// VAO/VBO
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glad_glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glad_glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glad_glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = nullptr;

// Instancing
PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor = nullptr;
PFNGLDRAWARRAYSINSTANCEDPROC glad_glDrawArraysInstanced = nullptr;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced = nullptr;

// Shaders
PFNGLCREATESHADERPROC glad_glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glad_glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glad_glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glad_glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glad_glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC glad_glUseProgram = nullptr;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = nullptr;
PFNGLDETACHSHADERPROC glad_glDetachShader = nullptr;

// Uniforms
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC glad_glUniform1i = nullptr;
PFNGLUNIFORM1FPROC glad_glUniform1f = nullptr;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = nullptr;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = nullptr;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = nullptr;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = nullptr;

#define LOAD(name) glad_##name = reinterpret_cast<decltype(glad_##name)>(loader(#name))

int gladLoadGLLoader(GLADloadproc loader) {
    if (!loader) return 0;

    // Core
    LOAD(glClear);
    LOAD(glClearColor);
    LOAD(glEnable);
    LOAD(glDisable);
    LOAD(glGetString);
    LOAD(glCullFace);
    LOAD(glFrontFace);
    LOAD(glDepthFunc);
    LOAD(glDepthMask);
    LOAD(glViewport);
    LOAD(glDrawArrays);
    LOAD(glDrawElements);
    LOAD(glBlendFunc);
    LOAD(glPixelStorei);
    LOAD(glPolygonMode);
    LOAD(glPolygonOffset);
    LOAD(glGetIntegerv);

    // Textures
    LOAD(glGenTextures);
    LOAD(glBindTexture);
    LOAD(glDeleteTextures);
    LOAD(glTexImage2D);
    LOAD(glTexParameteri);
    LOAD(glTexParameterf);
    LOAD(glActiveTexture);
    LOAD(glGenerateMipmap);

    // VAO/VBO
    LOAD(glGenVertexArrays);
    LOAD(glBindVertexArray);
    LOAD(glDeleteVertexArrays);
    LOAD(glGenBuffers);
    LOAD(glBindBuffer);
    LOAD(glBufferData);
    LOAD(glDeleteBuffers);
    LOAD(glVertexAttribPointer);
    LOAD(glEnableVertexAttribArray);

    // Instancing
    LOAD(glVertexAttribDivisor);
    LOAD(glDrawArraysInstanced);
    LOAD(glDrawElementsInstanced);

    // Shaders
    LOAD(glCreateShader);
    LOAD(glShaderSource);
    LOAD(glCompileShader);
    LOAD(glGetShaderiv);
    LOAD(glGetShaderInfoLog);
    LOAD(glDeleteShader);
    LOAD(glCreateProgram);
    LOAD(glAttachShader);
    LOAD(glLinkProgram);
    LOAD(glGetProgramiv);
    LOAD(glGetProgramInfoLog);
    LOAD(glUseProgram);
    LOAD(glDeleteProgram);
    LOAD(glDetachShader);

    // Uniforms
    LOAD(glGetUniformLocation);
    LOAD(glUniform1i);
    LOAD(glUniform1f);
    LOAD(glUniform2fv);
    LOAD(glUniform3fv);
    LOAD(glUniform4fv);
    LOAD(glUniformMatrix3fv);
    LOAD(glUniformMatrix4fv);

    GLVersion.major = 3;
    GLVersion.minor = 3;
    return 1;
}
