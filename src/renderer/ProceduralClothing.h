#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class Shader;
class ProceduralHumanoid;

enum class ClothingLayer {
    SHIRT_SIMPLE,
    SHIRT_FLANNEL,
    HOODIE,
    UNIFORM_SHIRT,
    KNIT_SWEATER,
    CREATURE_JACKET,
};

enum class TrouserStyle {
    CARGO_PANTS,
    SLIM_JEANS,
    HIKING_PANTS,
    GREY_TROUSERS,
    PLAIN_BLACK,
};

class ProceduralClothing {
public:
    struct ClothingVert {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
        glm::vec3 color;
    };

    ProceduralClothing(ClothingLayer shirtStyle, TrouserStyle trouserStyle);
    ~ProceduralClothing();

    void Build(ProceduralHumanoid* body);
    void Draw(Shader& shader, const glm::mat4& rootTransform) const;

private:
    ClothingLayer shirt;
    TrouserStyle trousers;

    GLuint shirtVAO = 0;
    GLuint shirtVBO = 0;
    GLuint shirtEBO = 0;
    GLuint trouserVAO = 0;
    GLuint trouserVBO = 0;
    GLuint trouserEBO = 0;
    int shirtIndexCount = 0;
    int trouserIndexCount = 0;

    std::vector<ClothingVert> shirtVerts;
    std::vector<unsigned int> shirtInds;
    std::vector<ClothingVert> trouserVerts;
    std::vector<unsigned int> trouserInds;

    void BuildShirt();
    void BuildTrousers();
    void UploadMesh(GLuint& vao, GLuint& vbo, GLuint& ebo, const std::vector<ClothingVert>& verts, const std::vector<unsigned int>& inds, int& outIndexCount);
};
