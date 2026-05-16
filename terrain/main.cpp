// Fromville Terrain Renderer - Standalone
// Horror atmosphere inspired by the TV show "From"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ============================================================
// Globals - Camera
// ============================================================
static const int SCR_WIDTH = 1920;
static const int SCR_HEIGHT = 1080;

static glm::vec3 cameraPos   = glm::vec3(0.0f, 1.7f, 0.0f);
static glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

static float yaw   = -90.0f;
static float pitch = 0.0f;
static bool  firstMouse = true;
static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;

static float deltaTime = 0.0f;
static float lastFrame = 0.0f;
static float dayTime = 0.0f; // 0.0-1.0, 0.5=noon, 0.0/1.0=midnight

static const float CAMERA_SPEED = 5.0f;
static const float MOUSE_SENSITIVITY = 0.1f;
static const float CAMERA_HEIGHT = 1.7f;

// ============================================================
// Callbacks
// ============================================================
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

static void mouse_callback(GLFWwindow*, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoff = (xpos - lastX) * MOUSE_SENSITIVITY;
    float yoff = (lastY - ypos) * MOUSE_SENSITIVITY;
    lastX = xpos; lastY = ypos;
    yaw   += xoff;
    pitch += yoff;
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

static void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float speed = CAMERA_SPEED * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flatFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * flatFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * flatFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= speed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += speed * right;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos.y += speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos.y -= speed;
    cameraPos.y = CAMERA_HEIGHT; // lock Y
}

// ============================================================
// Shader Utilities
// ============================================================
static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& source, const std::string& name) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "SHADER COMPILE ERROR [" << name << "]: " << log << std::endl;
    }
    return shader;
}

static GLuint createShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return 0;
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc, vertPath);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc, fragPath);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "SHADER LINK ERROR: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ============================================================
// Texture Loading
// ============================================================
static GLuint loadTexture(const std::string& path, bool srgb = false) {
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "ERROR: Failed to load texture: " << path << std::endl;
        return 0;
    }
    GLenum internalFmt, fmt;
    if (ch == 4)      { fmt = GL_RGBA; internalFmt = srgb ? GL_SRGB_ALPHA : GL_RGBA; }
    else if (ch == 3) { fmt = GL_RGB;  internalFmt = srgb ? GL_SRGB : GL_RGB; }
    else if (ch == 1) { fmt = GL_RED;  internalFmt = GL_RED; }
    else              { fmt = GL_RGB;  internalFmt = GL_RGB; }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    std::cout << "Loaded texture: " << path << " (" << w << "x" << h << ", " << ch << " ch)" << std::endl;
    return tex;
}

// ============================================================
// Ground Plane Setup
// ============================================================
struct GroundVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

static void createGroundPlane(GLuint& vao, GLuint& vbo, int& vertexCount) {
    // Single massive quad, 1000x1000 centered at origin
    // CCW winding when viewed from above (camera at Y=1.7)
    GroundVertex verts[6];
    glm::vec3 n(0.0f, 1.0f, 0.0f);
    // Triangle 1: BL -> TL -> TR (CCW from above)
    verts[0] = {{-500.0f, 0.0f, -500.0f}, n, {0.0f, 0.0f}};
    verts[1] = {{-500.0f, 0.0f,  500.0f}, n, {0.0f, 1.0f}};
    verts[2] = {{ 500.0f, 0.0f,  500.0f}, n, {1.0f, 1.0f}};
    // Triangle 2: BL -> TR -> BR (CCW from above)
    verts[3] = {{-500.0f, 0.0f, -500.0f}, n, {0.0f, 0.0f}};
    verts[4] = {{ 500.0f, 0.0f,  500.0f}, n, {1.0f, 1.0f}};
    verts[5] = {{ 500.0f, 0.0f, -500.0f}, n, {1.0f, 0.0f}};
    vertexCount = 6;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, normal));
    glEnableVertexAttribArray(1);
    // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, texcoord));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// ============================================================
// Grass Billboard Setup (Instanced)
// ============================================================
struct GrassVertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

static const int GRASS_COUNT = 100000;

static int grassTemplateVertCount = 0;

static void createGrassBillboard(GLuint& vao, GLuint& templateVBO, GLuint& instanceVBO) {
    // Single billboard quad per instance — camera-facing is handled by uBillboard matrix
    // BUG 3 fix: width=1.2, height=1.8, UV V clamped to 0.2..1.0 to skip empty top strip
    float w = 1.2f / 2.0f; // half-width = 0.6
    float h = 1.8f;
    GrassVertex quad[6] = {
        // bottom-left  (root) V=0.0 → bottom of original image = roots
        {{-w, 0.0f, 0.0f}, {0.0f, 0.0f}},
        // bottom-right (root) V=0.0
        {{ w, 0.0f, 0.0f}, {1.0f, 0.0f}},
        // top-right    (tip)  V=0.8 → 80% up original = tips, avoids empty top strip
        {{ w,  h,   0.0f}, {1.0f, 0.8f}},
        // bottom-left  (root)
        {{-w, 0.0f, 0.0f}, {0.0f, 0.0f}},
        // top-right    (tip)
        {{ w,  h,   0.0f}, {1.0f, 0.8f}},
        // top-left     (tip)  V=0.8
        {{-w,  h,   0.0f}, {0.0f, 0.8f}},
    };
    grassTemplateVertCount = 6;

    // Generate random positions (vec3: x, y, z)
    std::srand(42);
    std::vector<glm::vec3> instanceData(GRASS_COUNT);
    for (int i = 0; i < GRASS_COUNT; ++i) {
        float x = ((float)std::rand() / RAND_MAX) * 1000.0f - 500.0f;
        float z = ((float)std::rand() / RAND_MAX) * 1000.0f - 500.0f;
        instanceData[i] = glm::vec3(x, 0.0f, z);
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &templateVBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(vao);

    // Template quad VBO
    glBindBuffer(GL_ARRAY_BUFFER, templateVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    // location 0: position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), (void*)0);
    glEnableVertexAttribArray(0);
    // location 1: texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), (void*)offsetof(GrassVertex, texcoord));
    glEnableVertexAttribArray(1);

    // Instance VBO (vec3 position)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::vec3), instanceData.data(), GL_STATIC_DRAW);
    // location 2: instance position (vec3)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // advance once per instance

    glBindVertexArray(0);
}

// ============================================================
// Skydome Setup (procedural sphere)
// ============================================================
static void createSkydome(GLuint& vao, GLuint& vbo, GLuint& ebo, int& indexCount) {
    const int stacks = 16;
    const int slices = 24;
    const float radius = 450.0f;
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * glm::pi<float>() * float(j) / float(slices);
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            vertices.push_back(glm::vec3(x, y, z));
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    indexCount = (int)indices.size();
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ============================================================
// Main
// ============================================================
int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fromville", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 2. Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.07f, 1.0f);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // 3. Load shaders
    GLuint groundShader = createShaderProgram("shaders/ground.vert", "shaders/ground.frag");
    GLuint grassShader  = createShaderProgram("shaders/grass.vert",  "shaders/grass.frag");
    GLuint skyShader    = createShaderProgram("shaders/sky.vert",    "shaders/sky.frag");
    if (!groundShader || !grassShader || !skyShader) {
        std::cerr << "Failed to create shader programs" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 4. Load textures
    GLuint texAlbedo    = loadTexture("textures/coast_sand_rocks_02_diff_2k.jpg");
    GLuint texNormal    = loadTexture("textures/coast_sand_rocks_02_nor_gl_2k.png");
    GLuint texRoughness = loadTexture("textures/coast_sand_rocks_02_rough_2k.png");
    GLuint texGrass     = loadTexture("textures/vegetation_grass_card_03.png");

    // 5. Create ground plane
    GLuint groundVAO, groundVBO;
    int groundVertCount;
    createGroundPlane(groundVAO, groundVBO, groundVertCount);

    // 6-8. Create grass instances
    GLuint grassVAO, grassTemplateVBO, grassInstanceVBO;
    createGrassBillboard(grassVAO, grassTemplateVBO, grassInstanceVBO);

    // 9. Create skydome
    GLuint skyVAO, skyVBO, skyEBO;
    int skyIndexCount;
    createSkydome(skyVAO, skyVBO, skyEBO, skyIndexCount);

    // Static parameters
    float fogDensity = 0.004f;
    float tilingFactor = 80.0f;

    // FPS counter
    float fpsTimer = 0.0f;
    int frameCount = 0;

    // 10. Main render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS display
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            std::string title = "Fromville - FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            fpsTimer -= 1.0f;
        }

        glfwPollEvents();
        processInput(window);

        // ── Day/Night cycle ──
        float cycleSpeed = 1.0f / 120.0f;
        dayTime += deltaTime * cycleSpeed;
        if (dayTime > 1.0f) dayTime -= 1.0f;

        float sunAngle = dayTime * glm::two_pi<float>();
        glm::vec3 sunDir = glm::normalize(glm::vec3(cos(sunAngle), sin(sunAngle), 0.3f));
        float dayFactor = glm::clamp(sunDir.y * 2.5f, 0.0f, 1.0f);

        // Dynamic lighting
        glm::vec3 ambient = glm::mix(glm::vec3(0.04f,0.04f,0.07f), glm::vec3(0.35f,0.32f,0.28f), dayFactor);
        glm::vec3 lightColor = glm::mix(glm::vec3(0.3f,0.35f,0.5f), glm::vec3(1.0f,0.95f,0.80f), dayFactor);
        float diffuseStrength = glm::mix(0.3f, 0.85f, dayFactor);
        glm::vec3 fogColor = glm::mix(glm::vec3(0.02f,0.02f,0.04f), glm::vec3(0.55f,0.62f,0.72f), dayFactor);
        // Use sun during day, moon (opposite) at night for directional light
        glm::vec3 lightDir = sunDir.y > 0.0f ? sunDir : -sunDir;

        glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        float aspect = (fbH > 0) ? (float)fbW / (float)fbH : 1.0f;

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 2000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        // --- Draw Sky (first, no depth write) ---
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glUseProgram(skyShader);
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(skyShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(skyShader, "uView"), 1, GL_FALSE, glm::value_ptr(skyView));
        glUniformMatrix4fv(glGetUniformLocation(skyShader, "uModel"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform3fv(glGetUniformLocation(skyShader, "uSunDir"), 1, glm::value_ptr(sunDir));
        glUniform1f(glGetUniformLocation(skyShader, "uDayFactor"), dayFactor);
        glUniform1f(glGetUniformLocation(skyShader, "uDayTime"), dayTime);
        glBindVertexArray(skyVAO);
        glDrawElements(GL_TRIANGLES, skyIndexCount, GL_UNSIGNED_INT, 0);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        // --- Draw Ground ---
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glUseProgram(groundShader);
        glUniformMatrix4fv(glGetUniformLocation(groundShader, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(groundShader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(groundShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(groundShader, "uTilingFactor"), tilingFactor);
        glUniform3fv(glGetUniformLocation(groundShader, "uLightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(groundShader, "uLightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(groundShader, "uAmbient"), 1, glm::value_ptr(ambient));
        glUniform1f(glGetUniformLocation(groundShader, "uDiffuseStrength"), diffuseStrength);
        glUniform3fv(glGetUniformLocation(groundShader, "uViewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(groundShader, "uFogColor"), 1, glm::value_ptr(fogColor));
        glUniform1f(glGetUniformLocation(groundShader, "uFogDensity"), fogDensity);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texAlbedo);
        glUniform1i(glGetUniformLocation(groundShader, "uAlbedoMap"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texNormal);
        glUniform1i(glGetUniformLocation(groundShader, "uNormalMap"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texRoughness);
        glUniform1i(glGetUniformLocation(groundShader, "uRoughnessMap"), 2);
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, groundVertCount);
        glDisable(GL_POLYGON_OFFSET_FILL);

        // --- Draw Grass (instanced, no backface culling) ---
        glDisable(GL_CULL_FACE);
        glUseProgram(grassShader);
        glUniformMatrix4fv(glGetUniformLocation(grassShader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(grassShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(grassShader, "uTime"), currentFrame);
        glm::vec3 bbRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraFront));
        glm::vec3 bbUp    = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 bbLook  = glm::normalize(glm::cross(bbRight, bbUp));
        glm::mat3 billboardMatrix = glm::mat3(bbRight, bbUp, bbLook);
        glUniformMatrix3fv(glGetUniformLocation(grassShader, "uBillboard"), 1, GL_FALSE, glm::value_ptr(billboardMatrix));
        glUniform3fv(glGetUniformLocation(grassShader, "uLightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(grassShader, "uLightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(grassShader, "uAmbient"), 1, glm::value_ptr(ambient));
        glUniform1f(glGetUniformLocation(grassShader, "uDiffuseStrength"), diffuseStrength);
        glUniform3fv(glGetUniformLocation(grassShader, "uViewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(grassShader, "uFogColor"), 1, glm::value_ptr(fogColor));
        glUniform1f(glGetUniformLocation(grassShader, "uFogDensity"), fogDensity);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        glUniform1i(glGetUniformLocation(grassShader, "uGrassTexture"), 0);
        glBindVertexArray(grassVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, grassTemplateVertCount, GRASS_COUNT);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    glDeleteBuffers(1, &skyEBO);
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteBuffers(1, &grassTemplateVBO);
    glDeleteBuffers(1, &grassInstanceVBO);
    glDeleteTextures(1, &texAlbedo);
    glDeleteTextures(1, &texNormal);
    glDeleteTextures(1, &texRoughness);
    glDeleteTextures(1, &texGrass);
    glDeleteProgram(skyShader);
    glDeleteProgram(groundShader);
    glDeleteProgram(grassShader);
    glfwTerminate();
    return 0;
}
