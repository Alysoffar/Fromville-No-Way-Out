#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class Shader;
class ProceduralHumanoid;

enum class EyebrowStyle {
    HEAVY_FLAT,
    MEDIUM_ARCHED,
    MEDIUM,
    THIN_LIGHT,
};

enum class LipStyle {
    FULL,
    MEDIUM,
    THIN,
    NARROW,
};

enum class NoseStyle {
    BROAD_FLAT,
    MEDIUM_STRAIGHT,
    SMALL_UPTURNED,
    NARROW_LONG,
};

class FaceDetailGenerator {
public:
    FaceDetailGenerator();
    ~FaceDetailGenerator();

    void Build(ProceduralHumanoid* human, EyebrowStyle brow, LipStyle lip, NoseStyle nose);
    void Draw(Shader& shader, const glm::mat4& rootTransform) const;

private:
    struct FaceVert {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;

    std::vector<FaceVert> verts;
    std::vector<unsigned int> inds;

    void PushQuad(glm::vec3 center, glm::vec2 size, glm::vec3 normal);
    void Upload();
};
