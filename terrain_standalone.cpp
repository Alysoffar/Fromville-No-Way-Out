// Fromville - Main Game File
// Dependencies: GLFW3, GLAD, GLM
// Compile command: g++ terrain_standalone.cpp -lglfw -lGL -ldl -o Fromville

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cfloat>

// --- SECTION --- Shader Setup (Source)

const char* skyVertexSource = R"glsl(
#version 330 core
out vec3 viewRay;
uniform mat4 invViewRot;
uniform mat4 invProj;

void main() {
    vec2 pos;
    if(gl_VertexID == 0) pos = vec2(-1.0, -1.0);
    else if(gl_VertexID == 1) pos = vec2(3.0, -1.0);
    else pos = vec2(-1.0, 3.0);
    gl_Position = vec4(pos, 1.0, 1.0);
    
    vec4 rayClip = vec4(pos, -1.0, 1.0);
    vec4 rayEye = invProj * rayClip;
    rayEye = vec4(rayEye.xy, -1.0, 0.0);
    viewRay = (invViewRot * rayEye).xyz;
}
)glsl";

const char* skyFragmentSource = R"glsl(
#version 330 core
out vec4 FragColor;
in vec3 viewRay;

uniform vec3 lightDir;
uniform vec2 resolution;

void main() {
    vec3 dir = normalize(viewRay);
    
    vec3 skyBottom = vec3(0.74, 0.88, 0.97);
    vec3 skyTop = vec3(0.18, 0.38, 0.72);
    float yFactor = clamp(dir.y, 0.0, 1.0);
    vec3 skyColor = mix(skyBottom, skyTop, yFactor);
    
    vec3 lDir = normalize(lightDir);
    float sunDot = dot(dir, lDir);
    
    vec3 sunColor = vec3(1.0, 0.97, 0.85);
    float sunFactor = smoothstep(0.997, 0.999, sunDot);
    
    vec3 haloColor = vec3(1.0, 0.75, 0.3);
    float haloFactor = smoothstep(0.980, 0.997, sunDot);
    
    vec3 finalColor = skyColor + haloColor * haloFactor + sunColor * sunFactor;

    vec2 uv = gl_FragCoord.xy / resolution;
    
    float d1 = length((uv - vec2(0.3, 0.7)) * vec2(1.0, 2.5));
    float cloud1 = smoothstep(0.15, 0.05, d1);
    
    float d2 = length((uv - vec2(0.7, 0.6)) * vec2(1.0, 3.0));
    float cloud2 = smoothstep(0.12, 0.04, d2);
    
    float d3 = length((uv - vec2(0.5, 0.8)) * vec2(1.0, 2.0));
    float cloud3 = smoothstep(0.18, 0.08, d3);
    
    float totalCloud = clamp(cloud1 + cloud2 + cloud3, 0.0, 1.0);
    
    float cloudGrad = smoothstep(0.5, 0.9, uv.y);
    vec3 cloudColor = mix(vec3(0.88), vec3(1.0), cloudGrad);
    
    finalColor = mix(finalColor, cloudColor, totalCloud * 0.9);

    finalColor *= (1.0 - 0.4 * pow(length(uv - 0.5) * 1.6, 2.0));

    FragColor = vec4(finalColor, 1.0);
}
)glsl";

const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec2 resolution;

uniform float objectType;

void main()
{
    vec3 baseColor = vec3(1.0);
    bool applyHeightTint = false;
    bool applySlopeTint = false;

    if (objectType == 0.0) {
        // Terrain
        baseColor = vec3(0.3, 0.8, 0.2); 
        float pattern = sin(FragPos.x * 0.8) * sin(FragPos.z * 0.8);
        baseColor *= (1.0 + pattern * 0.1); 
        applyHeightTint = true;
        applySlopeTint = true;
    } else if (objectType == 1.0) {
        // Road
        baseColor = vec3(0.25, 0.24, 0.22);
    } else if (objectType == 2.0) {
        // House
        baseColor = vec3(0.45, 0.32, 0.21);
        applySlopeTint = true;
    } else if (objectType == 3.0) {
        // Mountains
        vec3 rockLow = vec3(0.40, 0.33, 0.25);
        vec3 rockHigh = vec3(0.78, 0.76, 0.74);
        float mountainMaxHeight = 90.0;
        float mountainH = clamp(FragPos.y / mountainMaxHeight, 0.0, 1.0);
        baseColor = mix(rockLow, rockHigh, mountainH);
        
        if (FragPos.y > 65.0) {
            float snowBlend = clamp((FragPos.y - 65.0) / 10.0, 0.0, 1.0);
            baseColor = mix(baseColor, vec3(0.95, 0.95, 0.97), snowBlend);
        }
    } else if (objectType == 4.0) {
        // Player Character
        baseColor = vec3(0.55, 0.60, 0.68);
    }

    vec3 albedo = baseColor;

    if (applyHeightTint) {
        vec3 colorLow = vec3(0.25, 0.55, 0.15);
        vec3 colorHigh = vec3(0.45, 0.72, 0.20);
        float hFactor = clamp(FragPos.y / 12.0, 0.0, 1.0);
        albedo *= mix(colorLow, colorHigh, hFactor);
    }

    vec3 norm = normalize(Normal);

    if (applySlopeTint) {
        vec3 slopeColorLow = vec3(0.40, 0.33, 0.25);
        vec3 slopeColorHigh = vec3(0.78, 0.76, 0.74);
        float slopeH = clamp(FragPos.y / 90.0, 0.0, 1.0);
        vec3 slopeColor = mix(slopeColorLow, slopeColorHigh, slopeH);

        float slope = 1.0 - max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);
        float slopeFactor = smoothstep(0.1, 0.4, slope); 
        albedo = mix(albedo, slopeColor, slopeFactor);
    }

    float ao = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 lDir = normalize(lightDir);
    vec3 ambient = 0.3 * albedo * ao;
    
    float diff = max(dot(norm, lDir), 0.0);
    vec3 diffuse = diff * albedo;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = vec3(0.05) * spec; 
    
    vec3 result = ambient + diffuse + specular;

    float distance = length(viewPos - FragPos);
    float fogStart = 60.0;
    float fogEnd = 150.0;
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.529, 0.808, 0.922); 
    
    result = mix(result, fogColor, fogFactor);

    vec2 uv = gl_FragCoord.xy / resolution;
    result *= (1.0 - 0.4 * pow(length(uv - 0.5) * 1.6, 2.0));

    FragColor = vec4(result, 1.0);
}
)glsl";

// --- SECTION --- Camera & Player
glm::vec3 playerPos = glm::vec3(0.0f, 0.0f, 0.0f);
float playerYaw = -90.0f;

float cameraOrbitYaw = 0.0f;
float cameraOrbitPitch = 20.0f;

glm::vec3 cameraPos   = glm::vec3(0.0f, 20.0f, 40.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
float fov   = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
  
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    playerYaw += xoffset; // Mouse turns the player
    cameraOrbitPitch += yoffset;

    if (cameraOrbitPitch > 60.0f) cameraOrbitPitch = 60.0f;
    if (cameraOrbitPitch < 5.0f) cameraOrbitPitch = 5.0f;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 10.0f * deltaTime;
    glm::vec3 forward(cos(glm::radians(playerYaw)), 0.0f, sin(glm::radians(playerYaw)));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        playerPos += speed * forward;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        playerPos -= speed * forward;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        playerPos -= speed * right;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        playerPos += speed * right;
    }
    
    playerPos.y = 0.0f;
}

// --- SECTION --- Terrain Generation
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// --- SECTION --- OBJ Loader
bool loadOBJ(const std::string& path, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open OBJ file: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::map<std::string, unsigned int> vertexCache;
    std::string line;
    glm::vec3 minBound(1e10f), maxBound(-1e10f);

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
            minBound = glm::min(minBound, pos);
            maxBound = glm::max(maxBound, pos);
        } else if (prefix == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        } else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        } else if (prefix == "f") {
            std::string vertexStr[4];
            int numVertices = 0;
            while (iss >> vertexStr[numVertices] && numVertices < 4) {
                numVertices++;
            }
            if (numVertices < 3) continue;
            
            auto processVertex = [&](const std::string& vStr) {
                if (vertexCache.count(vStr) > 0) {
                    outIndices.push_back(vertexCache[vStr]);
                } else {
                    unsigned int vertexIndex = 0, uvIndex = 0, normalIndex = 0;
                    size_t firstSlash = vStr.find('/');
                    size_t secondSlash = vStr.find('/', firstSlash + 1);
                    
                    if (firstSlash == std::string::npos) {
                        vertexIndex = std::stoi(vStr);
                    } else if (secondSlash == std::string::npos) {
                        vertexIndex = std::stoi(vStr.substr(0, firstSlash));
                        uvIndex = std::stoi(vStr.substr(firstSlash + 1));
                    } else if (firstSlash + 1 == secondSlash) {
                        vertexIndex = std::stoi(vStr.substr(0, firstSlash));
                        normalIndex = std::stoi(vStr.substr(secondSlash + 1));
                    } else {
                        vertexIndex = std::stoi(vStr.substr(0, firstSlash));
                        uvIndex = std::stoi(vStr.substr(firstSlash + 1, secondSlash - firstSlash - 1));
                        normalIndex = std::stoi(vStr.substr(secondSlash + 1));
                    }

                    Vertex vertex;
                    vertex.position = temp_positions[vertexIndex - 1];
                    vertex.uv = (uvIndex > 0 && uvIndex <= temp_uvs.size()) ? temp_uvs[uvIndex - 1] : glm::vec2(0.0f);
                    vertex.normal = (normalIndex > 0 && normalIndex <= temp_normals.size()) ? temp_normals[normalIndex - 1] : glm::vec3(0.0f);

                    outVertices.push_back(vertex);
                    unsigned int newIndex = outVertices.size() - 1;
                    outIndices.push_back(newIndex);
                    vertexCache[vStr] = newIndex;
                }
            };
            
            processVertex(vertexStr[0]);
            processVertex(vertexStr[1]);
            processVertex(vertexStr[2]);
            // Support Quad faces if found
            if (numVertices == 4) {
                processVertex(vertexStr[0]);
                processVertex(vertexStr[2]);
                processVertex(vertexStr[3]);
            }
        }
    }

    // Compute missing normals
    for (size_t i = 0; i < outIndices.size(); i += 3) {
        Vertex& v0 = outVertices[outIndices[i]];
        Vertex& v1 = outVertices[outIndices[i+1]];
        Vertex& v2 = outVertices[outIndices[i+2]];
        
        if (glm::length(v0.normal) < 0.001f || glm::length(v1.normal) < 0.001f || glm::length(v2.normal) < 0.001f) {
            glm::vec3 normal = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));
            if (glm::length(v0.normal) < 0.001f) v0.normal += normal;
            if (glm::length(v1.normal) < 0.001f) v1.normal += normal;
            if (glm::length(v2.normal) < 0.001f) v2.normal += normal;
        }
    }
    
    for (auto& v : outVertices) {
        if (glm::length(v.normal) > 0.001f) {
            v.normal = glm::normalize(v.normal);
        } else {
            v.normal = glm::vec3(0, 1, 0);
        }
    }

    // Center mesh around origin
    glm::vec3 center = (minBound + maxBound) * 0.5f;
    for (auto& v : outVertices) {
        v.position -= center;
    }

    return true;
}

float getNoise(float x, float z) {
    return 0.0f;
}

void generateTerrain(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    const int GRID_SIZE = 256;
    const int VERTEX_COUNT = GRID_SIZE + 1; 
    const float SIZE = 256.0f;
    
    vertices.resize(VERTEX_COUNT * VERTEX_COUNT);

    for(int z = 0; z < VERTEX_COUNT; z++) {
        for(int x = 0; x < VERTEX_COUNT; x++) {
            float worldX = ((float)x / GRID_SIZE) * SIZE - (SIZE / 2.0f);
            float worldZ = ((float)z / GRID_SIZE) * SIZE - (SIZE / 2.0f);
            float worldY = getNoise(worldX, worldZ);
            
            Vertex& v = vertices[z * VERTEX_COUNT + x];
            v.position = glm::vec3(worldX, worldY, worldZ);
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.uv = glm::vec2((float)x / GRID_SIZE * 32.0f, (float)z / GRID_SIZE * 32.0f);
        }
    }

    for(int z = 0; z < GRID_SIZE; z++) {
        for(int x = 0; x < GRID_SIZE; x++) {
            unsigned int topLeft = (z * VERTEX_COUNT) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = ((z + 1) * VERTEX_COUNT) + x;
            unsigned int bottomRight = bottomLeft + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    for(int i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::vec3(0.0f);
    }

    for(size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i+1];
        unsigned int i2 = indices[i+2];
        
        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;
        
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        if (normal.y < 0.0f) {
            normal = -normal;
        }
        
        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
    }

    for(int i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::normalize(vertices[i].normal);
    }
}

void generateMountains(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    const int N = 128;
    const float radius = 180.0f;
    const float baseHeight = -2.0f;
    
    for (int i = 0; i <= N; ++i) { 
        float angle = ((float)i / N) * 2.0f * glm::pi<float>();
        
        // 3 octaves of sine noise for peak height
        float hNoise = sin(angle * 5.0f) * 0.5f 
                     + sin(angle * 13.0f + 2.0f) * 0.35f 
                     + sin(angle * 27.0f + 4.0f) * 0.20f;
        float height = 65.0f + hNoise * 35.0f;
        
        float rNoise = sin(angle * 7.0f + 1.0f) * 0.5f + sin(angle * 19.0f) * 0.25f;
        float currentRadius = radius + rNoise * 15.0f;
        
        float x = cos(angle) * currentRadius;
        float z = sin(angle) * currentRadius;
        
        Vertex vBottom;
        vBottom.position = glm::vec3(x, baseHeight, z);
        vBottom.normal = glm::vec3(0.0f); 
        vBottom.uv = glm::vec2((float)i / N * 128.0f, 0.0f); 
        vertices.push_back(vBottom);
        
        float topRadius = currentRadius + 30.0f; 
        float topX = cos(angle) * topRadius;
        float topZ = sin(angle) * topRadius;
        
        Vertex vTop;
        vTop.position = glm::vec3(topX, height, topZ);
        vTop.normal = glm::vec3(0.0f);
        vTop.uv = glm::vec2((float)i / N * 128.0f, 8.0f); 
        vertices.push_back(vTop);
    }
    
    for (int i = 0; i < N; ++i) {
        unsigned int bottom0 = i * 2;
        unsigned int top0 = i * 2 + 1;
        unsigned int bottom1 = (i + 1) * 2;
        unsigned int top1 = (i + 1) * 2 + 1;
        
        indices.push_back(bottom0);
        indices.push_back(top1);
        indices.push_back(top0);
        
        indices.push_back(bottom0);
        indices.push_back(bottom1);
        indices.push_back(top1);
    }
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 v0 = vertices[indices[i]].position;
        glm::vec3 v1 = vertices[indices[i+1]].position;
        glm::vec3 v2 = vertices[indices[i+2]].position;
        
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        glm::vec3 centerDir = glm::vec3(-v0.x, 0.0f, -v0.z);
        if (glm::dot(normal, centerDir) < 0.0f) {
            normal = -normal; 
        }
        
        vertices[indices[i]].normal += normal;
        vertices[indices[i+1]].normal += normal;
        vertices[indices[i+2]].normal += normal;
    }
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (glm::length(vertices[i].normal) > 0.0001f) {
            vertices[i].normal = glm::normalize(vertices[i].normal);
        } else {
            vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

// Shader compiler helper
GLuint compileShader(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Error: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error: " << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader Program Link Error: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void setupVAO(GLuint& VAO, GLuint& VBO, GLuint& EBO, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

int main() {
    // --- SECTION --- Window Setup
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fromville", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
        glViewport(0, 0, width, height);
    });
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    GLuint shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
    GLuint skyShaderProgram = compileShader(skyVertexSource, skyFragmentSource);

    // 1. Terrain setup
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    generateTerrain(vertices, indices);
    GLuint VAO, VBO, EBO;
    setupVAO(VAO, VBO, EBO, vertices, indices);

    // 2. Mountains setup
    std::vector<Vertex> mtnVertices;
    std::vector<unsigned int> mtnIndices;
    generateMountains(mtnVertices, mtnIndices);
    GLuint mountainVAO, mountainVBO, mountainEBO;
    setupVAO(mountainVAO, mountainVBO, mountainEBO, mtnVertices, mtnIndices);

    // 3. Road setup
    std::vector<Vertex> roadVertices;
    std::vector<unsigned int> roadIndices;
    GLuint roadVAO = 0, roadVBO = 0, roadEBO = 0;
    std::vector<glm::mat4> roadTransforms;
    if (loadOBJ("assets/models/road.obj", roadVertices, roadIndices)) {
        setupVAO(roadVAO, roadVBO, roadEBO, roadVertices, roadIndices);
        for (int i = 0; i < 12; ++i) {
            glm::mat4 t = glm::mat4(1.0f);
            t = glm::translate(t, glm::vec3(0.0f, 0.01f, i * -4.0f));
            roadTransforms.push_back(t);
        }
    }

    // 4. House setup
    std::vector<Vertex> houseVertices;
    std::vector<unsigned int> houseIndices;
    GLuint houseVAO = 0, houseVBO = 0, houseEBO = 0;
    float houseMinY = 0.0f;
    if (loadOBJ("assets/models/Broken house.obj", houseVertices, houseIndices)) {
        setupVAO(houseVAO, houseVBO, houseEBO, houseVertices, houseIndices);
        
        float minY = FLT_MAX;
        for (auto& v : houseVertices) {
            minY = std::min(minY, v.position.y);
        }
        houseMinY = minY;
        std::cout << "House minY: " << houseMinY << std::endl;
    }
    
    // 5. Player setup
    std::vector<Vertex> playerVertices;
    std::vector<unsigned int> playerIndices;
    GLuint playerVAO = 0, playerVBO = 0, playerEBO = 0;
    float charMinY = 0.0f;
    if (loadOBJ("assets/models/boyd.obj", playerVertices, playerIndices)) {
        setupVAO(playerVAO, playerVBO, playerEBO, playerVertices, playerIndices);
        
        float minY = FLT_MAX;
        for(auto& v : playerVertices) {
            minY = std::min(minY, v.position.y);
        }
        charMinY = minY;
    }

    // Sky setup
    GLuint skyVAO;
    glGenVertexArrays(1, &skyVAO);
    glBindVertexArray(0);

    // --- SECTION --- Render Loop
    float fpsTimer = 0.0f;
    int frameCount = 0;

    glm::vec3 lightDir(0.6f, 1.0f, 0.4f);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            std::string title = "Fromville - FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            fpsTimer -= 1.0f;
        }

        processInput(window);

        // Recompute camera based on player
        float camDist = 12.0f;
        cameraPos.x = playerPos.x - camDist * cos(glm::radians(cameraOrbitPitch)) * cos(glm::radians(playerYaw + cameraOrbitYaw));
        cameraPos.y = playerPos.y + camDist * sin(glm::radians(cameraOrbitPitch));
        cameraPos.z = playerPos.z - camDist * cos(glm::radians(cameraOrbitPitch)) * sin(glm::radians(playerYaw + cameraOrbitYaw));
        
        if (cameraPos.y < playerPos.y + 1.5f) {
            cameraPos.y = playerPos.y + 1.5f;
        }
        
        cameraFront = glm::normalize(playerPos - cameraPos);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::vec2 resolution(static_cast<float>(width), static_cast<float>(height));

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // 1. Draw Sky
        glDepthMask(GL_FALSE);
        glUseProgram(skyShaderProgram);
        
        glm::mat4 invProj = glm::inverse(projection);
        glm::mat4 invViewRot = glm::inverse(glm::mat4(glm::mat3(view)));
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "invProj"), 1, GL_FALSE, glm::value_ptr(invProj));
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "invViewRot"), 1, GL_FALSE, glm::value_ptr(invViewRot));
        glUniform3fv(glGetUniformLocation(skyShaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform2fv(glGetUniformLocation(skyShaderProgram, "resolution"), 1, glm::value_ptr(resolution));
        
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);

        // Standard Shader
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform2fv(glGetUniformLocation(shaderProgram, "resolution"), 1, glm::value_ptr(resolution));

        // 2. Terrain
        glUniform1f(glGetUniformLocation(shaderProgram, "objectType"), 0.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        
        // 3. Mountains
        glUniform1f(glGetUniformLocation(shaderProgram, "objectType"), 3.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glBindVertexArray(mountainVAO);
        glDrawElements(GL_TRIANGLES, mtnIndices.size(), GL_UNSIGNED_INT, 0);

        // 4. Road Tiles
        if (roadVAO != 0) {
            glUniform1f(glGetUniformLocation(shaderProgram, "objectType"), 1.0f);
            glBindVertexArray(roadVAO);
            for (const auto& t : roadTransforms) {
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(t));
                glDrawElements(GL_TRIANGLES, roadIndices.size(), GL_UNSIGNED_INT, 0);
            }
        }

        // 5. Broken House
        if (houseVAO != 0) {
            glUniform1f(glGetUniformLocation(shaderProgram, "objectType"), 2.0f);
            glm::mat4 houseModel = glm::mat4(1.0f);
            houseModel = glm::translate(houseModel, glm::vec3(0.0f, -houseMinY + 0.01f, -50.0f));
            houseModel = glm::rotate(houseModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // houseModel = glm::scale(houseModel, glm::vec3(1.0f)); // Adjust scale here if necessary
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(houseModel));
            glBindVertexArray(houseVAO);
            glDrawElements(GL_TRIANGLES, houseIndices.size(), GL_UNSIGNED_INT, 0);
        }

        // 6. Player Character
        if (playerVAO != 0) {
            glUniform1f(glGetUniformLocation(shaderProgram, "objectType"), 4.0f);

            // Align character feet to terrain using the character's lowest vertex
            glm::mat4 charModel = glm::translate(glm::mat4(1.0f), glm::vec3(playerPos.x, playerPos.y - charMinY, playerPos.z));

            // Apply Y rotation for playerYaw
            charModel = glm::rotate(charModel, glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));

            // Apply scale if any (none)
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(charModel));
            glBindVertexArray(playerVAO);
            glDrawElements(GL_TRIANGLES, playerIndices.size(), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    glDeleteVertexArrays(1, &mountainVAO);
    glDeleteBuffers(1, &mountainVBO);
    glDeleteBuffers(1, &mountainEBO);

    if (roadVAO != 0) {
        glDeleteVertexArrays(1, &roadVAO);
        glDeleteBuffers(1, &roadVBO);
        glDeleteBuffers(1, &roadEBO);
    }
    if (houseVAO != 0) {
        glDeleteVertexArrays(1, &houseVAO);
        glDeleteBuffers(1, &houseVBO);
        glDeleteBuffers(1, &houseEBO);
    }
    if (playerVAO != 0) {
        glDeleteVertexArrays(1, &playerVAO);
        glDeleteBuffers(1, &playerVBO);
        glDeleteBuffers(1, &playerEBO);
    }
    
    glDeleteVertexArrays(1, &skyVAO);

    glDeleteProgram(shaderProgram);
    glDeleteProgram(skyShaderProgram);

    glfwTerminate();
    return 0;
}
