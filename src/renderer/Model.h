#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "Mesh.h"

class Model {
public:
	explicit Model(const std::string& path, bool flipUVs = false);
	void Draw(Shader& shader);
	void DrawInstanced(Shader& shader, int count);

	std::vector<Mesh> meshes;
	std::string directory;

	struct BoneInfo {
		glm::mat4 offsetMatrix;
		int id;
	};

	std::unordered_map<std::string, BoneInfo> boneInfoMap;
	int boneCount = 0;
	glm::mat4 globalInverseTransform;
	const aiScene* scene;

private:
	void LoadModel(const std::string& path);
	void ProcessNode(aiNode* node, const aiScene* sceneData);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* sceneData);
	std::vector<MeshTexture> LoadMaterialTextures(aiMaterial* material, aiTextureType type, const std::string& typeName);
	static glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix);

	Assimp::Importer importer;
	bool flipUVs;
};