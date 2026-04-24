#include "Model.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include <stb_image.h>

#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <glad/glad.h>

namespace {
	struct TextureCacheEntry {
		GLuint id;
		int width;
		int height;
		int channels;
	};

	std::unordered_map<std::string, MeshTexture> textureCache;

	glm::mat4 IdentityMatrix() {
		return glm::mat4(1.0f);
	}

} // namespace

Model::Model(const std::string& path, bool flipUVsValue)
	: globalInverseTransform(IdentityMatrix()), scene(nullptr), flipUVs(flipUVsValue) {
	LoadModel(path);
}

void Model::LoadModel(const std::string& path) {
	unsigned int flags = aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_LimitBoneWeights;
	if (flipUVs) {
		flags |= aiProcess_FlipUVs;
	}

	scene = importer.ReadFile(path, flags);
	if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
		throw std::runtime_error("Model load failed for '" + path + "': " + importer.GetErrorString());
	}

	directory = path.substr(0, path.find_last_of("/\\"));
	globalInverseTransform = glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation));
	ProcessNode(scene->mRootNode, scene);
	std::cout << "[model] '" << path << "' loaded with " << boneCount << " bones." << std::endl;
}

void Model::Draw(Shader& shader) {
	for (const Mesh& mesh : meshes) {
		mesh.Draw(shader);
	}
}

void Model::DrawInstanced(Shader& shader, int count) {
	for (const Mesh& mesh : meshes) {
		mesh.DrawInstanced(shader, count);
	}
}

void Model::ProcessNode(aiNode* node, const aiScene* sceneData) {
	for (unsigned int index = 0; index < node->mNumMeshes; ++index) {
		aiMesh* mesh = sceneData->mMeshes[node->mMeshes[index]];
		meshes.push_back(ProcessMesh(mesh, sceneData));
	}

	for (unsigned int index = 0; index < node->mNumChildren; ++index) {
		ProcessNode(node->mChildren[index], sceneData);
	}
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* sceneData) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<MeshTexture> textures;

	vertices.reserve(mesh->mNumVertices);
	for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
		Vertex vertex;
		vertex.Position = glm::vec3(mesh->mVertices[index].x, mesh->mVertices[index].y, mesh->mVertices[index].z);
		vertex.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[index].x, mesh->mNormals[index].y, mesh->mNormals[index].z) : glm::vec3(0.0f, 1.0f, 0.0f);
		if (mesh->mTextureCoords[0]) {
			vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][index].x, mesh->mTextureCoords[0][index].y);
		} else {
			vertex.TexCoords = glm::vec2(0.0f);
		}
		vertex.Tangent = mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mTangents[index].x, mesh->mTangents[index].y, mesh->mTangents[index].z) : glm::vec3(1.0f, 0.0f, 0.0f);
		vertex.Bitangent = mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mBitangents[index].x, mesh->mBitangents[index].y, mesh->mBitangents[index].z) : glm::vec3(0.0f, 1.0f, 0.0f);
		vertices.push_back(vertex);
	}

	for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
		const aiFace face = mesh->mFaces[faceIndex];
		for (unsigned int index = 0; index < face.mNumIndices; ++index) {
			indices.push_back(face.mIndices[index]);
		}
	}

	if (mesh->mMaterialIndex < sceneData->mNumMaterials) {
		aiMaterial* material = sceneData->mMaterials[mesh->mMaterialIndex];
		auto appendTextures = [&](aiTextureType type, const std::string& typeName) {
			std::vector<MeshTexture> loadedTextures = LoadMaterialTextures(material, type, typeName);
			textures.insert(textures.end(), loadedTextures.begin(), loadedTextures.end());
		};

		appendTextures(aiTextureType_DIFFUSE, "albedoMap");
		appendTextures(aiTextureType_NORMALS, "normalMap");
		appendTextures(aiTextureType_SHININESS, "roughnessMap");
		appendTextures(aiTextureType_AMBIENT, "aoMap");
		appendTextures(aiTextureType_EMISSIVE, "emissiveMap");
	}

	for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone* bone = mesh->mBones[boneIndex];
		std::string boneName = bone->mName.C_Str();
		size_t mixamoPos = boneName.find("mixamorig");
		if (mixamoPos != std::string::npos) {
			size_t sepPos = boneName.find_first_of(":_", mixamoPos);
			if (sepPos != std::string::npos) {
				boneName = boneName.substr(sepPos + 1);
			}
		}
		BoneInfo boneInfo;
		auto found = boneInfoMap.find(boneName);
		if (found == boneInfoMap.end()) {
			boneInfo.id = boneCount++;
			boneInfo.offsetMatrix = ConvertMatrix(bone->mOffsetMatrix);
			boneInfoMap.emplace(boneName, boneInfo);
		} else {
			boneInfo = found->second;
		}

		for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
			const aiVertexWeight weight = bone->mWeights[weightIndex];
			if (weight.mVertexId >= vertices.size()) {
				continue;
			}

			Vertex& vertex = vertices[weight.mVertexId];
			for (int slot = 0; slot < 4; ++slot) {
				if (vertex.BoneWeights[slot] == 0.0f) {
					vertex.BoneIDs[slot] = boneInfo.id;
					vertex.BoneWeights[slot] = weight.mWeight;
					break;
				}
			}
		}
	}

	for (Vertex& vertex : vertices) {
		float weightSum = vertex.BoneWeights.x + vertex.BoneWeights.y + vertex.BoneWeights.z + vertex.BoneWeights.w;
		if (weightSum > 0.001f) {
			vertex.BoneWeights /= weightSum;
		} else {
			vertex.BoneIDs = glm::ivec4(0, 0, 0, 0);
			vertex.BoneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	return Mesh(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<MeshTexture> Model::LoadMaterialTextures(aiMaterial* material, aiTextureType type, const std::string& typeName) {
	std::vector<MeshTexture> textures;
	const unsigned int count = material->GetTextureCount(type);
	for (unsigned int index = 0; index < count; ++index) {
		aiString texturePath;
		aiString matName;
		material->Get(AI_MATKEY_NAME, matName);
		if (material->GetTexture(type, index, &texturePath) != AI_SUCCESS) {
			continue;
		}

		const std::string fullPath = directory.empty() ? std::string(texturePath.C_Str()) : (directory + "/" + texturePath.C_Str());
		auto cacheIt = textureCache.find(fullPath);
		if (cacheIt != textureCache.end()) {
			textures.push_back(cacheIt->second);
			continue;
		}

		stbi_set_flip_vertically_on_load(flipUVs ? 1 : 0);
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);
		if (!data) {
			std::cerr << "Failed to load texture: " << fullPath << '\n';
			continue;
		}
		
		std::cout << "Loaded material '" << matName.C_Str() << "' texture: " << fullPath << "\n";

		GLenum format = GL_RGB;
		if (channels == 1) {
			format = GL_RED;
		} else if (channels == 3) {
			format = GL_RGB;
		} else if (channels == 4) {
			format = GL_RGBA;
		}

		GLuint textureID = 0;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);

		MeshTexture meshTexture;
		meshTexture.id = textureID;
		meshTexture.type = typeName;
		meshTexture.path = fullPath;
		textureCache.emplace(fullPath, meshTexture);
		textures.push_back(meshTexture);
	}

	return textures;
}

glm::mat4 Model::ConvertMatrix(const aiMatrix4x4& from) {
	glm::mat4 to(1.0f);
	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
	return to;
}