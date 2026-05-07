#include "game/world/World.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/terrain.h"
#include "engine/resources/loader.h"
#include "game/world/map_manager.h"

// =============================================================================
// World Object / Instance System
// =============================================================================

namespace {

struct WorldMesh {
    Mesh mesh;
    float minY = 0.0f;
    bool valid = false;
};

struct WorldInstance {
    int meshIndex;
    glm::mat4 transform;
};

// All unique meshes
enum MeshID {
    MESH_ROAD = 0,
    MESH_ROAD_CROSS,
    MESH_ROAD_CORNER,
    MESH_ROAD_CORNER_LONG,
    MESH_ROAD_END,
    MESH_BROKEN_HOUSE,
    MESH_OLD_HOUSE,
    MESH_LIGHT_STAND,
    MESH_TREE1,
    MESH_TREE2,
    MESH_TREE3,
    MESH_GRASS,
    MESH_PLAYER,
    MESH_COUNT
};

const char* gMeshPaths[MESH_COUNT] = {
    "assets/models/road.obj",
    "assets/models/Road crosssection.obj",
    "assets/models/Road routat.obj",
    "assets/models/Road routat long.obj",
    "assets/models/Road end.obj",
    "assets/models/Broken house.obj",
    "assets/models/Old houseobj.obj",
    "assets/models/light stand.obj",
    "assets/models/Tree.obj",
    "assets/models/Tree2.obj",
    "assets/models/Tree3.obj",
    "assets/models/Grass.obj",
    "assets/models/boyd.obj"
};

WorldMesh gMeshes[MESH_COUNT];
std::vector<WorldInstance> gInstances;
Shader gWorldShader("WorldObjects");
bool gWorldReady = false;

float CalculateMinY(const std::vector<MeshVertex>& vertices) {
    float minY = FLT_MAX;
    for (const auto& v : vertices) {
        minY = std::min(minY, v.position.y);
    }
    return minY;
}

// Build a model matrix: translate -> rotate Y -> scale
glm::mat4 MakeTransform(glm::vec3 pos, float minY, float rotYDeg = 0.0f, float scale = 1.0f) {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, glm::vec3(pos.x, pos.y - minY + 0.01f, pos.z));
    if (rotYDeg != 0.0f) {
        m = glm::rotate(m, glm::radians(rotYDeg), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (scale != 1.0f) {
        m = glm::scale(m, glm::vec3(scale));
    }
    return m;
}

void LoadAllMeshes() {
    if (gWorldReady) return;

    std::cout << "[World] Loading all world meshes...\n";

    for (int i = 0; i < MESH_COUNT; ++i) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        
        if (Loader::LoadOBJ(gMeshPaths[i], vertices, indices)) {
            gMeshes[i].mesh.Create(vertices, indices);
            gMeshes[i].minY = CalculateMinY(vertices);
            gMeshes[i].valid = gMeshes[i].mesh.IsValid();
            std::cout << "  [OK] " << gMeshPaths[i] 
                      << " (minY=" << gMeshes[i].minY << ")\n";
        } else {
            std::cerr << "  [FAIL] Could not load: " << gMeshPaths[i] << "\n";
            gMeshes[i].valid = false;
        }
    }

    gWorldShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
    gWorldReady = true;
}

void BuildWorldLayout() {
    gInstances.clear();
    gInstances.reserve(128);

    auto addInst = [](int meshId, glm::vec3 pos, float rotY = 0.0f, float scale = 1.0f) {
        if (!gMeshes[meshId].valid) return;
        WorldInstance inst;
        inst.meshIndex = meshId;
        inst.transform = MakeTransform(pos, gMeshes[meshId].minY, rotY, scale);
        gInstances.push_back(inst);
    };

    // ================================================================
    // ROADS — Main Street (Z = 0 to Z = -65)
    // ================================================================
    for (int i = 0; i < 14; ++i) {
        addInst(MESH_ROAD, glm::vec3(0.0f, 0.0f, i * -5.0f));
    }

    // Intersection at Z = -70
    addInst(MESH_ROAD_CROSS, glm::vec3(0.0f, 0.0f, -70.0f));

    // Corner piece where main street meets side road
    addInst(MESH_ROAD_CORNER, glm::vec3(0.0f, 0.0f, -70.0f), -90.0f);

    // Side road going left (negative X) from the intersection
    for (int i = 1; i <= 8; ++i) {
        addInst(MESH_ROAD, glm::vec3(i * -5.0f, 0.0f, -70.0f), 90.0f);
    }

    // Road end caps
    addInst(MESH_ROAD_END, glm::vec3(0.0f, 0.0f, 5.0f), 180.0f);     // Start of Main Street
    addInst(MESH_ROAD_END, glm::vec3(-45.0f, 0.0f, -70.0f), 90.0f);   // End of side road

    // ================================================================
    // BUILDINGS
    // ================================================================
    
    // Broken house at end of Main Street
    addInst(MESH_BROKEN_HOUSE, glm::vec3(0.0f, 0.0f, -80.0f), 180.0f);

    // Old houses — left side of Main Street
    addInst(MESH_OLD_HOUSE, glm::vec3(-12.0f, 0.0f, -20.0f), 90.0f);
    addInst(MESH_OLD_HOUSE, glm::vec3(-12.0f, 0.0f, -40.0f), 90.0f);
    addInst(MESH_OLD_HOUSE, glm::vec3(-12.0f, 0.0f, -60.0f), 90.0f);

    // Old houses — right side of Main Street
    addInst(MESH_OLD_HOUSE, glm::vec3(12.0f, 0.0f, -25.0f), -90.0f);
    addInst(MESH_OLD_HOUSE, glm::vec3(12.0f, 0.0f, -50.0f), -90.0f);

    // ================================================================
    // STREET LAMPS — both sides of Main Street
    // ================================================================
    for (int i = 0; i < 7; ++i) {
        float z = i * -10.0f;
        // Left side
        addInst(MESH_LIGHT_STAND, glm::vec3(-4.0f, 0.0f, z), 90.0f);
        // Right side
        addInst(MESH_LIGHT_STAND, glm::vec3(4.0f, 0.0f, z), -90.0f);
    }

    // ================================================================
    // TREES — scattered around the world edges
    // ================================================================
    struct TreePlacement { int meshId; glm::vec3 pos; };
    TreePlacement trees[] = {
        // Tree.obj (8 copies)
        { MESH_TREE1, { 35.0f, 0.0f,  -10.0f } },
        { MESH_TREE1, { -40.0f, 0.0f, -15.0f } },
        { MESH_TREE1, { 45.0f, 0.0f,  -35.0f } },
        { MESH_TREE1, { -50.0f, 0.0f, -50.0f } },
        { MESH_TREE1, { 55.0f, 0.0f,  -60.0f } },
        { MESH_TREE1, { -35.0f, 0.0f, -85.0f } },
        { MESH_TREE1, { 40.0f, 0.0f,  -90.0f } },
        { MESH_TREE1, { -60.0f, 0.0f, -30.0f } },
        // Tree2.obj (6 copies)
        { MESH_TREE2, { 50.0f, 0.0f,  5.0f } },
        { MESH_TREE2, { -45.0f, 0.0f, -5.0f } },
        { MESH_TREE2, { 60.0f, 0.0f,  -45.0f } },
        { MESH_TREE2, { -55.0f, 0.0f, -65.0f } },
        { MESH_TREE2, { 30.0f, 0.0f,  -80.0f } },
        { MESH_TREE2, { -30.0f, 0.0f, -95.0f } },
        // Tree3.obj (5 copies)
        { MESH_TREE3, { 65.0f, 0.0f,  -20.0f } },
        { MESH_TREE3, { -65.0f, 0.0f, -40.0f } },
        { MESH_TREE3, { 55.0f, 0.0f,  -75.0f } },
        { MESH_TREE3, { -70.0f, 0.0f, -10.0f } },
        { MESH_TREE3, { 70.0f, 0.0f,  -55.0f } },
    };

    for (int i = 0; i < 19; ++i) {
        float rotY = i * 37.5f; // Varied rotation per tree
        addInst(trees[i].meshId, trees[i].pos, rotY);
    }

    // ================================================================
    // GRASS — scattered on open terrain
    // ================================================================
    struct GrassSpot { float x, z; };
    GrassSpot grassPositions[] = {
        { 20.0f, -8.0f }, { -20.0f, -12.0f }, { 25.0f, -22.0f }, { -25.0f, -28.0f },
        { 18.0f, -38.0f }, { -18.0f, -42.0f }, { 22.0f, -52.0f }, { -22.0f, -58.0f },
        { 30.0f, -68.0f }, { -28.0f, -72.0f }, { 15.0f, -78.0f }, { -15.0f, -82.0f },
        { 28.0f, -5.0f },  { -32.0f, -35.0f }, { 35.0f, -48.0f }, { -38.0f, -55.0f },
        { 42.0f, -15.0f }, { -42.0f, -25.0f }, { 48.0f, -65.0f }, { -48.0f, -75.0f },
    };

    for (int i = 0; i < 20; ++i) {
        float rotY = i * 18.0f;
        addInst(MESH_GRASS, glm::vec3(grassPositions[i].x, 0.0f, grassPositions[i].z), rotY, 0.5f);
    }

    std::cout << "[World] Built " << gInstances.size() << " world instances.\n";
}

} // anonymous namespace

// =============================================================================
// World class implementation
// =============================================================================

World::World() = default;

void World::Initialize() {
    static MapManager worldMap;
    static TerrainRenderer terrainRenderer;

    mapManager = &worldMap;
    terrain = &terrainRenderer;

    terrain->Initialize();
    player.transform.position = glm::vec3(0.0f, 2.0f, 10.0f);
    
    LoadAllMeshes();
    BuildWorldLayout();

    // Inject terrain procedural mesh into collision
    collisionWorld.AddTriangles(terrain->GetTriangles());
    std::cout << "[World] Terrain collision added successfully!\n";

    // Wire collision system to entities
    player.SetCollisionWorld(&collisionWorld);
}


void World::Update(const Camera& camera, float dt) {
    if (!mapManager || !terrain) {
        return;
    }

    player.Update(dt);
    
    mapManager->Update(camera.GetPosition());
    terrain->Update(*mapManager);
}

void World::Render(const Camera& camera, float aspectRatio) {
    if (!terrain) {
        return;
    }

    // 1. Sky + Terrain + Mountains (handled by terrain renderer)
    terrain->Render(camera, aspectRatio);

    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::vec3 lightDir = glm::vec3(0.6f, 1.0f, 0.4f);
    const glm::vec3 viewPos = camera.GetPosition();

    // 2. All world instances (roads, buildings, lamps, trees, grass)
    if (gWorldReady) {
        gWorldShader.Bind();
        gWorldShader.SetMat4("projection", projection);
        gWorldShader.SetMat4("view", view);
        gWorldShader.SetVec3("lightDir", lightDir);
        gWorldShader.SetVec3("viewPos", viewPos);

        for (const auto& inst : gInstances) {
            const WorldMesh& wm = gMeshes[inst.meshIndex];
            if (!wm.valid) continue;
            
            gWorldShader.SetMat4("model", inst.transform);
            wm.mesh.Draw();
        }

        gWorldShader.Unbind();
    }

    // 3. Player character (drawn last, on top)
    if (gMeshes[MESH_PLAYER].valid) {
        glm::vec3 pos = player.transform.position;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), 
            glm::vec3(pos.x, pos.y - gMeshes[MESH_PLAYER].minY, pos.z));
        model = glm::rotate(model, glm::radians(player.transform.rotation.y), 
            glm::vec3(0.0f, 1.0f, 0.0f));

        gWorldShader.Bind();
        gWorldShader.SetMat4("projection", projection);
        gWorldShader.SetMat4("view", view);
        gWorldShader.SetMat4("model", model);
        gWorldShader.SetVec3("lightDir", lightDir);
        gWorldShader.SetVec3("viewPos", viewPos);
        gMeshes[MESH_PLAYER].mesh.Draw();
        gWorldShader.Unbind();
    }
}