#include <glad/glad.h>

#include <GLFW/glfw3.h>

struct gladGLversionStruct GLVersion = { 0, 0 };

PFNGLCLEARPROC glad_glClear = nullptr;
PFNGLCLEARCOLORPROC glad_glClearColor = nullptr;
PFNGLENABLEPROC glad_glEnable = nullptr;
PFNGLGETSTRINGPROC glad_glGetString = nullptr;
PFNGLCULLFACEPROC glad_glCullFace = nullptr;
PFNGLDISABLEPROC glad_glDisable = nullptr;
PFNGLFRONTFACEPROC glad_glFrontFace = nullptr;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = nullptr;
PFNGLVIEWPORTPROC glad_glViewport = nullptr;
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glad_glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glad_glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glad_glBufferData = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = nullptr;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = nullptr;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = nullptr;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = nullptr;

PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = nullptr;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = nullptr;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = nullptr;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = nullptr;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = nullptr;
PFNGLUNIFORM1FPROC glad_glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glad_glUniform1i = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = nullptr;
PFNGLUSEPROGRAMPROC glad_glUseProgram = nullptr;
PFNGLDETACHSHADERPROC glad_glDetachShader = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = nullptr;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = nullptr;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = nullptr;
PFNGLATTACHSHADERPROC glad_glAttachShader = nullptr;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = nullptr;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = nullptr;
PFNGLDELETESHADERPROC glad_glDeleteShader = nullptr;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = nullptr;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = nullptr;
PFNGLCOMPILESHADERPROC glad_glCompileShader = nullptr;
PFNGLSHADERSOURCEPROC glad_glShaderSource = nullptr;
PFNGLCREATESHADERPROC glad_glCreateShader = nullptr;

static void LoadCoreFunctions(GLADloadproc loader) {
	glad_glClear = reinterpret_cast<PFNGLCLEARPROC>(loader("glClear"));
	glad_glClearColor = reinterpret_cast<PFNGLCLEARCOLORPROC>(loader("glClearColor"));
	glad_glEnable = reinterpret_cast<PFNGLENABLEPROC>(loader("glEnable"));
	glad_glGetString = reinterpret_cast<PFNGLGETSTRINGPROC>(loader("glGetString"));
	glad_glCullFace = reinterpret_cast<PFNGLCULLFACEPROC>(loader("glCullFace"));
	glad_glDisable = reinterpret_cast<PFNGLDISABLEPROC>(loader("glDisable"));
	glad_glFrontFace = reinterpret_cast<PFNGLFRONTFACEPROC>(loader("glFrontFace"));
	glad_glDepthFunc = reinterpret_cast<PFNGLDEPTHFUNCPROC>(loader("glDepthFunc"));
	glad_glViewport = reinterpret_cast<PFNGLVIEWPORTPROC>(loader("glViewport"));
	glad_glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(loader("glGenVertexArrays"));
	glad_glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(loader("glBindVertexArray"));
	glad_glDeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(loader("glDeleteVertexArrays"));
	glad_glGenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(loader("glGenBuffers"));
	glad_glBindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(loader("glBindBuffer"));
	glad_glBufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(loader("glBufferData"));
	glad_glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(loader("glVertexAttribPointer"));
	glad_glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(loader("glEnableVertexAttribArray"));
	glad_glDeleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(loader("glDeleteBuffers"));
	glad_glDrawArrays = reinterpret_cast<PFNGLDRAWARRAYSPROC>(loader("glDrawArrays"));
	glad_glDrawElements = reinterpret_cast<PFNGLDRAWELEMENTSPROC>(loader("glDrawElements"));

	glad_glUniformMatrix4fv = reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(loader("glUniformMatrix4fv"));
	glad_glUniformMatrix3fv = reinterpret_cast<PFNGLUNIFORMMATRIX3FVPROC>(loader("glUniformMatrix3fv"));
	glad_glUniform4fv = reinterpret_cast<PFNGLUNIFORM4FVPROC>(loader("glUniform4fv"));
	glad_glUniform3fv = reinterpret_cast<PFNGLUNIFORM3FVPROC>(loader("glUniform3fv"));
	glad_glUniform2fv = reinterpret_cast<PFNGLUNIFORM2FVPROC>(loader("glUniform2fv"));
	glad_glUniform1f = reinterpret_cast<PFNGLUNIFORM1FPROC>(loader("glUniform1f"));
	glad_glUniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(loader("glUniform1i"));
	glad_glGetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(loader("glGetUniformLocation"));
	glad_glUseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(loader("glUseProgram"));
	glad_glDetachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(loader("glDetachShader"));
	glad_glGetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(loader("glGetProgramInfoLog"));
	glad_glGetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(loader("glGetProgramiv"));
	glad_glLinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(loader("glLinkProgram"));
	glad_glAttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(loader("glAttachShader"));
	glad_glCreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(loader("glCreateProgram"));
	glad_glDeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(loader("glDeleteProgram"));
	glad_glDeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(loader("glDeleteShader"));
	glad_glGetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(loader("glGetShaderInfoLog"));
	glad_glGetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(loader("glGetShaderiv"));
	glad_glCompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(loader("glCompileShader"));
	glad_glShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(loader("glShaderSource"));
	glad_glCreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(loader("glCreateShader"));
}

int gladLoadGLLoader(GLADloadproc loader) {
	if (!loader) {
		return 0;
	}

	LoadCoreFunctions(loader);
	GLVersion.major = 4;
	GLVersion.minor = 5;
	return 1;
}