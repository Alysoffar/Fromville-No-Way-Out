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
#include "game/dialogue/DialogueManager.h"
#include "game/moral/MoralCorruptionSystem.h"
#include "game/memory/MemoryReplaySystem.h"
#include "game/story/StoryManager.h"
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <cctype>

// =============================================================================
// Player Mesh
// =============================================================================

namespace {

struct WorldMesh {
    Mesh mesh;
    float minY = 0.0f;
    bool valid = false;
};

struct VoiceCueLibrary {
    bool initialized = false;
    std::unordered_map<std::string, std::string> monster;
    std::unordered_map<std::string, std::string> npcFemale;
    std::unordered_map<std::string, std::string> npcMale;
    std::unordered_map<std::string, std::string> responses;
    std::unordered_map<std::string, std::string> panicFemale;
    std::unordered_map<std::string, std::string> panicMale;
};

VoiceCueLibrary& GetVoiceCueLibrary() {
    static VoiceCueLibrary library;
    return library;
}

std::string BuildVoiceSlug(const std::string& line) {
    std::string cleaned;
    cleaned.reserve(line.size());
    bool prevUnderscore = false;
    for (char c : line) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) {
            cleaned.push_back(static_cast<char>(std::tolower(uc)));
            prevUnderscore = false;
        } else if (!prevUnderscore) {
            cleaned.push_back('_');
            prevUnderscore = true;
        }
    }
    while (!cleaned.empty() && cleaned.front() == '_') cleaned.erase(cleaned.begin());
    while (!cleaned.empty() && cleaned.back() == '_') cleaned.pop_back();
    if (cleaned.size() > 40) {
        cleaned = cleaned.substr(0, 40);
        while (!cleaned.empty() && cleaned.back() == '_') cleaned.pop_back();
    }
    return cleaned.empty() ? "line" : cleaned;
}

std::string NormalizeVoiceLine(std::string line) {
    const std::string monsterPrefix = "[MONSTER]";
    if (line.rfind(monsterPrefix, 0) == 0) {
        line = line.substr(monsterPrefix.size());
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) line.erase(line.begin());
    }
    return line;
}

std::string StripSpeakerPrefix(const std::string& line) {
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos || colon + 1 >= line.size()) return line;
    std::string stripped = line.substr(colon + 1);
    while (!stripped.empty() && std::isspace(static_cast<unsigned char>(stripped.front()))) stripped.erase(stripped.begin());
    return stripped;
}

void RegisterVoiceFolder(AudioManager& audio, const std::filesystem::path& folder, const std::string& cuePrefix, std::unordered_map<std::string, std::string>& outMap) {
    if (!std::filesystem::exists(folder)) return;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".wav") continue;
        const std::string stem = entry.path().stem().string();
        const std::size_t underscore = stem.find('_');
        if (underscore == std::string::npos || underscore + 1 >= stem.size()) continue;
        const std::string slug = stem.substr(underscore + 1);
        const std::string cueName = cuePrefix + "/" + entry.path().filename().string();
        if (audio.LoadSound(cueName, entry.path().string())) outMap[slug] = cueName;
    }
}

void InitializeVoiceCueLibrary(AudioManager& audio) {
    VoiceCueLibrary& library = GetVoiceCueLibrary();
    if (library.initialized) return;
    RegisterVoiceFolder(audio, "assets/audio/voice/monster", "voice/monster", library.monster);
    RegisterVoiceFolder(audio, "assets/audio/voice/npc_female", "voice/npc_female", library.npcFemale);
    RegisterVoiceFolder(audio, "assets/audio/voice/npc_male", "voice/npc_male", library.npcMale);
    RegisterVoiceFolder(audio, "assets/audio/voice/responses", "voice/responses", library.responses);
    RegisterVoiceFolder(audio, "assets/audio/voice/panic_female", "voice/panic_female", library.panicFemale);
    RegisterVoiceFolder(audio, "assets/audio/voice/panic_male", "voice/panic_male", library.panicMale);
    library.initialized = true;
}

bool PlayVoiceLineInternal(AudioManager* audio, const std::string& group, const std::string& rawLine, float gain = 0.85f) {
    if (!audio || rawLine.empty()) return false;
    VoiceCueLibrary& library = GetVoiceCueLibrary();
    if (!library.initialized) InitializeVoiceCueLibrary(*audio);
    std::string line = NormalizeVoiceLine(rawLine);
    if (group == "npc_female" || group == "npc_male") line = StripSpeakerPrefix(line);
    const std::string slug = BuildVoiceSlug(line);
    const std::unordered_map<std::string, std::string>* table = nullptr;
    if (group == "monster") table = &library.monster;
    else if (group == "npc_female") table = &library.npcFemale;
    else if (group == "npc_male") table = &library.npcMale;
    else if (group == "responses") table = &library.responses;
    else if (group == "panic_female") table = &library.panicFemale;
    else if (group == "panic_male") table = &library.panicMale;
    if (!table) return false;
    const auto found = table->find(slug);
    if (found == table->end()) return false;
    return audio->PlaySound(found->second, gain);
}
} // anonymous namespace

bool PlayVoiceLine(AudioManager* audio, const std::string& group, const std::string& rawLine, float gain) {
    return PlayVoiceLineInternal(audio, group, rawLine, gain);
}

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

// =============================================================================
// World class implementation
// =============================================================================

World::World() = default;

void World::Initialize() {
    // Initialize characters
    characters.push_back(std::make_unique<Boyd>(glm::vec3(0.0f, 2.0f, 10.0f)));
    characters.push_back(std::make_unique<Jade>(glm::vec3(5.0f, 2.0f, 10.0f)));
    characters.push_back(std::make_unique<Tabitha>(glm::vec3(-5.0f, 2.0f, 10.0f)));
    characters.push_back(std::make_unique<Victor>(glm::vec3(10.0f, 2.0f, 10.0f)));
    characters.push_back(std::make_unique<Sara>(glm::vec3(-10.0f, 2.0f, 10.0f)));
    
    activeCharacterIndex = 0;

    LoadMeshes(&collisionWorld, doors);

    // Wire collision system to entities
    for (auto& character : characters) {
        character->SetCollisionWorld(&collisionWorld);
    }

    // Initialize systems
    audioManager = std::make_unique<AudioManager>();
    audioManager->Initialize();

    questSystem = std::make_unique<QuestSystem>(&worldClock);
    
    std::array<Character*, 5> charPtrs;
    for(int i=0; i<5; ++i) charPtrs[i] = characters[i].get();
    questSystem->Initialize(charPtrs);
    
    puzzleManager.Initialize();
    interactionSystem.Initialize();
    DialogueManager::Instance().Initialize();
    DialogueManager::Instance().LoadConversationsFromFolder();

    std::cout << "[World] Initialized (player + houses + " << doors.size() << " doors).\n";
}

void World::Update(const Camera& camera, float dt, InputManager& input) {
    (void)camera;
    // Update active character
    GetPlayer().Update(dt);
    
    // Update inactive characters
    for (size_t i = 0; i < characters.size(); ++i) {
        if (static_cast<int>(i) != activeCharacterIndex) {
            characters[i]->Update(dt);
        }
    }

    // Update doors
    for (auto& door : doors) {
        door.Update(dt);
    }

    // Update gameplay systems
    inputContext.SetInputManager(&input);
    
    worldClock += dt;
    if (questSystem) questSystem->Update(dt);
    puzzleManager.Update(dt, inputContext);
    interactionSystem.Update(dt, *questSystem);
    
    DialogueManager::Instance().Update(dt);
    DialogueManager::Instance().HandleInput(inputContext);
    
    StoryManager::Instance().Update(dt);
    MoralCorruptionSystem::Instance().Update(dt);
    MemoryReplaySystem::Instance().Update(dt);
}

void World::TryInteract() {
    glm::vec3 playerPos = GetPlayer().transform.position;

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
            GetPlayer().transform.position = door.GetInsidePosition();
            
            isInsideBuilding = true;
            break;
        }
    }
}

void World::TryExit() {
    if (!isInsideBuilding) return;

    std::cout << "[World] Exiting building...\n";
    
    // Teleport active character back to where they were standing
    GetPlayer().transform.position = previousOutsidePosition;
    
    isInsideBuilding = false;
}

void World::SwitchCharacter(int index) {
    if (index < 0 || index >= static_cast<int>(characters.size())) return;
    
    GetPlayer().OnSwitchedFrom();
    activeCharacterIndex = index;
    GetPlayer().OnSwitchedTo();
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

std::string World::GetInteractionPrompt() const {
    return interactionSystem.GetPromptFor(GetPlayer(), *questSystem, false, GetPlayer().GetType());
}

bool World::NearestInteractionIsPickup() const {
    return false; // Implement properly if Pickup type is added
}

std::string World::GetQuestHelperText() const {
    if (!questSystem) return "";
    const auto* activeQuest = questSystem->GetCharacterQuest(GetPlayer().GetType());
    if (activeQuest) return activeQuest->GetTitle();
    return "";
}

std::string World::GetQuestWaypointText() const {
    return ""; // Implement if waypoints are added
}

bool World::IsPuzzleActive() const {
    return puzzleManager.IsActive();
}

void World::UpdatePuzzle(float dt, const InputContext& input) {
    puzzleManager.Update(dt, input);
}

bool World::HasActiveQuest() const {
    if (!questSystem) return false;
    return questSystem->GetCharacterQuest(GetPlayer().GetType()) != nullptr;
}

CharacterType World::GetActiveQuestCharacter() const {
    return GetPlayer().GetType();
}

World::DayCyclePhase World::GetCyclePhase() const {
    // Basic mapping based on worldClock or similar if available
    return DayCyclePhase::Morning;
}

void World::RenderNarrativeOverlays(TextRenderer& renderer, int width, int height) {
    // Implement narrative overlay rendering
}

void World::RenderPuzzleOverlay(TextRenderer& renderer, int width, int height) {
    if (puzzleManager.IsActive()) {
        puzzleManager.Render(renderer, width, height);
    }
}

bool World::ConsumeSpawnRestartRequest() {
    bool requested = spawnRestartRequested;
    spawnRestartRequested = false;
    return requested;
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