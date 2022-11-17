#pragma once
#include <string>

#include <DirectXMath.h>
#include <wrl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>

#include "Mesh.h"

using namespace Microsoft::WRL;
using namespace DirectX;

class CommandList;

class Model
{
public:
	Model(const std::string& file_path, CommandList& commandList);

	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList);

	void LoadVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<UINT>& indices);

private:
	Assimp::Importer mImporter;
	const aiScene* pScene = nullptr;

public:
	std::vector<Mesh> mMeshes;
	std::string name;
};