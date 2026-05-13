#include "game/world/World.h"

#include <cfloat>
#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/resources/loader.h"
#include "game/world/DayNightCycle.h"

// =============================================================================
// Player Mesh
// =============================================================================

namespace {

struct WorldMesh {
    Mesh mesh;
    float minY = 0.0f;
    bool valid = false;
};

WorldMesh gPlayerMesh;
WorldMesh gHouseMesh;
WorldMesh gDinnerMesh;
WorldMesh gPoliceMesh;
Shader gPlayerShader("Player");
bool gPlayerReady = false;

float CalculateMinY(const std::vector<MeshVertex>& vertices) {
    float minY = FLT_MAX;
    for (const auto& v : vertices) {
        minY = std::min(minY, v.position.y);
    }
    return minY;
}

void LoadMeshes() {
    if (gPlayerReady) return;

    std::cout << "[World] Loading meshes...\n";

    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    if (Loader::LoadOBJ("assets/models/boyd.obj", vertices, indices)) {
        gPlayerMesh.mesh.Create(vertices, indices);
        gPlayerMesh.minY = CalculateMinY(vertices);
        gPlayerMesh.valid = gPlayerMesh.mesh.IsValid();
        std::cout << "  [OK] Player mesh loaded (minY=" << gPlayerMesh.minY << ")\n";
    } else {
        std::cerr << "  [FAIL] Could not load player mesh.\n";
        gPlayerMesh.valid = false;
    }

    vertices.clear();
    indices.clear();
    if (Loader::LoadOBJ("assets/models/House.obj", vertices, indices)) {
        gHouseMesh.mesh.Create(vertices, indices);
        gHouseMesh.minY = CalculateMinY(vertices);
        gHouseMesh.valid = gHouseMesh.mesh.IsValid();
        std::cout << "  [OK] House mesh loaded (minY=" << gHouseMesh.minY << ")\n";
    } else {
        std::cerr << "  [FAIL] Could not load house mesh.\n";
        gHouseMesh.valid = false;
    }

    vertices.clear();
    indices.clear();
    if (Loader::LoadOBJ("assets/models/Dinner.obj", vertices, indices)) {
        gDinnerMesh.mesh.Create(vertices, indices);
        gDinnerMesh.minY = CalculateMinY(vertices);
        gDinnerMesh.valid = gDinnerMesh.mesh.IsValid();
        std::cout << "  [OK] Dinner mesh loaded (minY=" << gDinnerMesh.minY << ")\n";
    } else {
        std::cerr << "  [FAIL] Could not load dinner mesh.\n";
        gDinnerMesh.valid = false;
    }

    vertices.clear();
    indices.clear();
    if (Loader::LoadOBJ("assets/models/Police.obj", vertices, indices)) {
        gPoliceMesh.mesh.Create(vertices, indices);
        gPoliceMesh.minY = CalculateMinY(vertices);
        gPoliceMesh.valid = gPoliceMesh.mesh.IsValid();
        std::cout << "  [OK] Police mesh loaded (minY=" << gPoliceMesh.minY << ")\n";
    } else {
        std::cerr << "  [FAIL] Could not load police mesh.\n";
        gPoliceMesh.valid = false;
    }

    gPlayerShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
    gPlayerReady = true;
}

} // anonymous namespace

// =============================================================================
// World class implementation
// =============================================================================

World::World() = default;

void World::Initialize() {
    player.transform.position = glm::vec3(0.0f, 2.0f, 10.0f);

    LoadMeshes();

    // Wire collision system to entities
    player.SetCollisionWorld(&collisionWorld);

    std::cout << "[World] Initialized (player + 2 houses).\n";
}

void World::Update(const Camera& camera, float dt) {
    (void)camera;
    player.Update(dt);
}

void World::Render(const Camera& camera, float aspectRatio) {
    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::vec3 lightDir = glm::vec3(0.6f, 1.0f, 0.4f);
    const glm::vec3 viewPos = camera.GetPosition();

    if (gPlayerMesh.valid) {
        glm::vec3 pos = player.transform.position;
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(pos.x, pos.y - gPlayerMesh.minY, pos.z));
        model = glm::rotate(model, glm::radians(player.transform.rotation.y),
            glm::vec3(0.0f, 1.0f, 0.0f));

        gPlayerShader.Bind();
        gPlayerShader.SetMat4("projection", projection);
        gPlayerShader.SetMat4("view", view);
        gPlayerShader.SetMat4("model", model);
        gPlayerShader.SetVec3("lightDir", lightDir);
        gPlayerShader.SetVec3("viewPos", viewPos);
        gPlayerMesh.mesh.Draw();
        
        if (gHouseMesh.valid) {
            // House 1
            glm::mat4 hModel1 = glm::translate(glm::mat4(1.0f), glm::vec3(-25.0f, -gHouseMesh.minY + 0.05f, 0.0f));
            hModel1 = glm::rotate(hModel1, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", hModel1);
            gHouseMesh.mesh.Draw();

            // House 2
            glm::mat4 hModel2 = glm::translate(glm::mat4(1.0f), glm::vec3(25.0f, -gHouseMesh.minY + 0.05f, 0.0f));
            hModel2 = glm::rotate(hModel2, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", hModel2);
            gHouseMesh.mesh.Draw();
        }

        if (gDinnerMesh.valid) {
            // Dinner
            glm::mat4 dModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gDinnerMesh.minY * 2.0f + 0.05f, -40.0f));
            dModel = glm::rotate(dModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            dModel = glm::scale(dModel, glm::vec3(2.0f));
            gPlayerShader.SetMat4("model", dModel);
            gDinnerMesh.mesh.Draw();
        }

        if (gPoliceMesh.valid) {
            // Police
            glm::mat4 pModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gPoliceMesh.minY * 2.0f + 0.05f, 40.0f));
            pModel = glm::rotate(pModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            pModel = glm::scale(pModel, glm::vec3(2.0f));
            gPlayerShader.SetMat4("model", pModel);
            gPoliceMesh.mesh.Draw();
        }

        gPlayerShader.Unbind();
    }
}

void World::RenderObjects(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight) {
    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::vec3 lightDir = dayNight.getActiveLightDir();
    const glm::vec3 viewPos = camera.GetPosition();

    // Player character
    if (gPlayerMesh.valid) {
        glm::vec3 pos = player.transform.position;
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(pos.x, pos.y - gPlayerMesh.minY, pos.z));
        model = glm::rotate(model, glm::radians(player.transform.rotation.y),
            glm::vec3(0.0f, 1.0f, 0.0f));

        gPlayerShader.Bind();
        gPlayerShader.SetMat4("projection", projection);
        gPlayerShader.SetMat4("view", view);
        gPlayerShader.SetMat4("model", model);
        gPlayerShader.SetVec3("lightDir", lightDir);
        gPlayerShader.SetVec3("viewPos", viewPos);
        gPlayerMesh.mesh.Draw();

        if (gHouseMesh.valid) {
            // House 1
            glm::mat4 hModel1 = glm::translate(glm::mat4(1.0f), glm::vec3(-25.0f, -gHouseMesh.minY + 0.05f, 0.0f));
            hModel1 = glm::rotate(hModel1, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", hModel1);
            gHouseMesh.mesh.Draw();

            // House 2
            glm::mat4 hModel2 = glm::translate(glm::mat4(1.0f), glm::vec3(25.0f, -gHouseMesh.minY + 0.05f, 0.0f));
            hModel2 = glm::rotate(hModel2, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", hModel2);
            gHouseMesh.mesh.Draw();
        }

        if (gDinnerMesh.valid) {
            // Dinner
            glm::mat4 dModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gDinnerMesh.minY * 2.0f + 0.05f, -40.0f));
            dModel = glm::rotate(dModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            dModel = glm::scale(dModel, glm::vec3(2.0f));
            gPlayerShader.SetMat4("model", dModel);
            gDinnerMesh.mesh.Draw();
        }

        if (gPoliceMesh.valid) {
            // Police
            glm::mat4 pModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gPoliceMesh.minY * 2.0f + 0.05f, 40.0f));
            pModel = glm::rotate(pModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            pModel = glm::scale(pModel, glm::vec3(2.0f));
            gPlayerShader.SetMat4("model", pModel);
            gPoliceMesh.mesh.Draw();
        }

        gPlayerShader.Unbind();
    }
}