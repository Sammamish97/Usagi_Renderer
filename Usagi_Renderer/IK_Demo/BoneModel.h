#pragma once
#include <string>
#include <map>

#include <DirectXMath.h>
#include <wrl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>

class CommandList;
class BoneModel
{
public:
	BoneModel(const std::string& file_path, CommandList& commandList);
	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);

private:
	UINT mBoneCounter = 0;

public:
	std::string name;
	Assimp::Importer mImporter;
	const aiScene* pScene = nullptr;

	std::vector<DirectX::XMFLOAT4> mvertices;
};

