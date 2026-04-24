#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class Shader;
class ProceduralHumanoid;

enum class HairStyle {
    CLOSE_CROP,
    WAVY_MEDIUM,
    PONYTAIL_LOOSE,
    LONGISH_SWEPT,
    LONG_STRAIGHT,
    CREATURE_MATTED,
};

class ProceduralHair {
public:
    ProceduralHair(HairStyle hairStyle, glm::vec3 base, glm::vec3 highlight, float strandLength);
    ~ProceduralHair();

    void GenerateStrands(ProceduralHumanoid* humanoid);
    void Update(float dt, glm::vec3 characterVelocity);
    void Draw(Shader& shader, const glm::mat4& rootTransform) const;

private:
    struct HairVert {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
    };

    HairStyle style;
    glm::vec3 baseColor;
    glm::vec3 highlightColor;
    float length;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;

    std::vector<HairVert> verts;
    std::vector<unsigned int> inds;

    void BuildMesh();
};
