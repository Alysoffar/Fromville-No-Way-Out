#include "game/world/World.h"

#include <cfloat>
#include <iostream>
#include <vector>
#include <map>

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

void LoadMeshes(CollisionWorld* cw, std::vector<Door>& doors) {
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

    auto LoadBuilding = [&](const std::string& path, WorldMesh& outMesh, glm::vec3 pos, float rot, float scale, const std::string& doorKeyword, float doorOpenAngle) {
        std::vector<OBJShape> shapes;
        if (!Loader::LoadOBJWithShapes(path, shapes)) return;

        std::vector<MeshVertex> mainVertices;
        std::vector<unsigned int> mainIndices;

        // First pass: Find global minY to correctly position everything, and compute local bounds
        float globalMinY = FLT_MAX;
        glm::vec3 localMin(FLT_MAX);
        glm::vec3 localMax(-FLT_MAX);
        for (const auto& s : shapes) {
            for (const auto& v : s.vertices) {
                globalMinY = std::min(globalMinY, v.position.y);
                localMin = glm::min(localMin, v.position * scale);
                localMax = glm::max(localMax, v.position * scale);
            }
        }
        float adjustedY = -globalMinY * scale + 0.05f;
        glm::vec3 buildingPos = glm::vec3(pos.x, adjustedY, pos.z);

        // Compute true world center of the building geometry
        glm::vec3 localCenter = (localMin + localMax) * 0.5f;
        glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
        localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 buildingWorldCenter = glm::vec3(localToWorld * glm::vec4(localCenter, 1.0f));
        buildingWorldCenter.y = buildingPos.y + 0.5f; // place slightly above floor

        std::map<std::string, std::pair<std::vector<MeshVertex>, std::vector<unsigned int>>> doorGroups;

        for (auto& s : shapes) {
            if (s.name.find(doorKeyword) != std::string::npos &&
                s.name.find("Frame") == std::string::npos &&
                s.name.find("Sill") == std::string::npos) {
                
                std::string groupName = s.name;
                if (s.name.find("Knob") != std::string::npos) {
                    groupName = "Door_Main_Mesh"; // Merge knob with main door
                }
                
                auto& group = doorGroups[groupName];
                unsigned int offset = static_cast<unsigned int>(group.first.size());
                for (auto v : s.vertices) {
                    if (scale != 1.0f) v.position *= scale;
                    group.first.push_back(v);
                }
                for (auto i : s.indices) group.second.push_back(i + offset);
            } else {
                unsigned int offset = static_cast<unsigned int>(mainVertices.size());
                for (auto v : s.vertices) {
                    if (scale != 1.0f) v.position *= scale;
                    mainVertices.push_back(v);
                }
                for (auto i : s.indices) mainIndices.push_back(i + offset);
            }
        }
        
        for (auto& [dName, dData] : doorGroups) {
            if (dData.first.empty()) continue;
            
            glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
            for (const auto& v : dData.first) {
                minP = glm::min(minP, v.position);
                maxP = glm::max(maxP, v.position);
            }
            
            // If it's a right door, hinge is on the right (max X). Otherwise left (min X).
            float hingeX = (dName.find("_R") != std::string::npos) ? maxP.x : minP.x;
            glm::vec3 hingePos = glm::vec3(hingeX, 0.0f, minP.z);
            
            // Fix double doors so they swing outward correctly (reverse angle for right doors)
            float angle = doorOpenAngle;
            if (dName.find("_R") != std::string::npos) {
                angle = -doorOpenAngle;
            }
            
            // Calculate door center in world space
            glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
            localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 doorWorldCenter = glm::vec3(localToWorld * glm::vec4(hingePos, 1.0f));
            
            Door d;
            // Store buildingPos as the position for rendering transform
            d.Initialize(dName, dData.first, dData.second, buildingPos, rot, angle, hingePos, doorWorldCenter);
            
            // We teleport the player exactly to the building's physical center
            d.SetInsidePosition(buildingWorldCenter);
            doors.push_back(std::move(d));
        }

        // If no doors were extracted (like Diner or Police), create a virtual door at the entrance
        if (doorGroups.empty()) {
            glm::vec3 entranceLocalPos(0.0f);
            int stepVertCount = 0;
            for (const auto& s : shapes) {
                if (s.name.find("Step") != std::string::npos) {
                    for (const auto& v : s.vertices) {
                        entranceLocalPos += v.position * scale;
                        stepVertCount++;
                    }
                }
            }
            
            if (stepVertCount > 0) {
                entranceLocalPos /= static_cast<float>(stepVertCount);
            }

            glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
            localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 entranceWorldPos = glm::vec3(localToWorld * glm::vec4(entranceLocalPos, 1.0f));

            Door virtualDoor;
            virtualDoor.Initialize(path + "_Entrance", {}, {}, buildingPos, rot, 0.0f, glm::vec3(0.0f), entranceWorldPos);
            
            // Teleport point inside: center of building
            glm::vec3 insidePos = buildingPos;
            insidePos.y += 0.5f; // safely above floor
            virtualDoor.SetInsidePosition(insidePos);
            
            doors.push_back(std::move(virtualDoor));
        }
        
        outMesh.mesh.Create(mainVertices, mainIndices);
        outMesh.minY = CalculateMinY(mainVertices);
        outMesh.valid = outMesh.mesh.IsValid();

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, adjustedY, pos.z));
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        // We already scaled the vertices, so we don't scale the model matrix anymore!
        if (cw) cw->AddTrianglesFromMesh(mainVertices, mainIndices, model, true);
    };

    // House 1 & 2
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(-25.0f, 0, 0.0f), 90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(25.0f, 0, 0.0f), -90.0f, 1.0f, "Door_Main", 90.0f);

    LoadBuilding("assets/models/Dinner.obj", gDinnerMesh, glm::vec3(0.0f, 0, -40.0f), 0.0f, 2.0f, "DoorGlass", -90.0f);
    LoadBuilding("assets/models/Police.obj", gPoliceMesh, glm::vec3(0.0f, 0, 40.0f), 180.0f, 2.0f, "Door", 90.0f);

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

    LoadMeshes(&collisionWorld, doors);

    // Wire collision system to entities
    player.SetCollisionWorld(&collisionWorld);

    std::cout << "[World] Initialized (player + houses + " << doors.size() << " doors).\n";
}

void World::Update(const Camera& camera, float dt) {
    (void)camera;
    player.Update(dt);
    for (auto& door : doors) {
        door.Update(dt);
    }
}

void World::TryInteract() {
    glm::vec3 playerPos = player.transform.position;

    if (isInsideBuilding) {
        for (auto& door : doors) {
            // Check distance to the inside position to allow exiting
            if (glm::distance(playerPos, door.GetInsidePosition()) < 10.0f) {
                TryExit();
                return;
            }
        }
        // Fallback: If they somehow got stuck far away, let them exit anyway
        TryExit();
        return;
    }
    
    for (auto& door : doors) {
        float dist = glm::distance(playerPos, door.GetPosition());
        if (dist < 4.0f) { // Require player to be close to the door
            std::cout << "[World] Entering building through: " << door.GetName() << " | Dist: " << dist << "\n";
            
            // Save the player's exact outside position before entering
            previousOutsidePosition = playerPos;
            
            // Teleport player directly to the building's physical center
            player.transform.position = door.GetInsidePosition();
            
            isInsideBuilding = true;
            break;
        }
    }
}

void World::TryExit() {
    if (!isInsideBuilding) return;

    std::cout << "[World] Exiting building...\n";
    
    // Teleport player back to where they were standing
    player.transform.position = previousOutsidePosition;
    
    isInsideBuilding = false;
}


void World::Render(const Camera& camera, float aspectRatio) {
    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::vec3 lightDir = glm::vec3(0.6f, 1.0f, 0.4f);
    const glm::vec3 viewPos = camera.GetPosition();

    if (gPlayerMesh.valid) {
        gPlayerShader.Bind();
        gPlayerShader.SetMat4("projection", projection);
        gPlayerShader.SetMat4("view", view);
        gPlayerShader.SetVec3("lightDir", lightDir);
        gPlayerShader.SetVec3("viewPos", viewPos);
        
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
            glm::mat4 dModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gDinnerMesh.minY + 0.05f, -40.0f));
            dModel = glm::rotate(dModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", dModel);
            gDinnerMesh.mesh.Draw();
        }

        if (gPoliceMesh.valid) {
            // Police
            glm::mat4 pModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gPoliceMesh.minY + 0.05f, 40.0f));
            pModel = glm::rotate(pModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", pModel);
            gPoliceMesh.mesh.Draw();
        }

        gPlayerShader.Unbind();
    }
}

void World::RenderObjects(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight, float fogDensity) {
    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::vec3 lightDir = dayNight.getActiveLightDir();
    const glm::vec3 lightColor = dayNight.getLightColor();
    const glm::vec3 ambient = dayNight.getAmbientColor();
    float diffuseStrength = dayNight.getDiffuseStrength();
    const glm::vec3 fogColor = dayNight.getFogColor();
    const glm::vec3 viewPos = camera.GetPosition();

    // Player character
    if (gPlayerMesh.valid) {
        gPlayerShader.Bind();
        gPlayerShader.SetMat4("projection", projection);
        gPlayerShader.SetMat4("view", view);
        
        gPlayerShader.SetVec3("uLightDir", lightDir);
        gPlayerShader.SetVec3("uLightColor", lightColor);
        gPlayerShader.SetVec3("uAmbient", ambient);
        gPlayerShader.SetFloat("uDiffuseStrength", diffuseStrength);
        gPlayerShader.SetVec3("uViewPos", viewPos);
        gPlayerShader.SetVec3("uFogColor", fogColor);
        gPlayerShader.SetFloat("uFogDensity", fogDensity);

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
            glm::mat4 dModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gDinnerMesh.minY + 0.05f, -40.0f));
            dModel = glm::rotate(dModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", dModel);
            gDinnerMesh.mesh.Draw();
        }

        if (gPoliceMesh.valid) {
            // Police
            glm::mat4 pModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gPoliceMesh.minY + 0.05f, 40.0f));
            pModel = glm::rotate(pModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gPlayerShader.SetMat4("model", pModel);
            gPoliceMesh.mesh.Draw();
        }

        // Doors
        for (auto& door : doors) {
            door.Render(gPlayerShader, view, projection, lightDir, lightColor, ambient, diffuseStrength, viewPos, fogColor, fogDensity);
        }

        gPlayerShader.Unbind();
    }
}