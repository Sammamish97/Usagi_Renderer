#include "BoneModel.h"
BoneModel::BoneModel(const std::string& file_path, CommandList& commandList)
{
	LoadModel(file_path, commandList);
}
void BoneModel::LoadModel(const std::string& file_path, CommandList& commandList)
{
    mImporter.SetPropertyBool(AI_CONFIG_FBX_USE_SKELETON_BONE_CONTAINER, true);

    pScene = mImporter.ReadFile(file_path, aiProcess_ConvertToLeftHanded);

    if (!pScene || !pScene->mRootNode)
    {
        assert("Fail to Load %s", file_path.c_str());
    }
    name = file_path.substr(0, file_path.find_last_of('/'));
    ProcessNode(pScene->mRootNode, pScene, commandList);
}
void BoneModel::ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList)
{
    static std::vector<aiMatrix4x4> boneMat;
    static std::vector<aiVector3t<float>> bonePos;
    static std::vector<aiQuaterniont<float>> boneRot;
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        auto current_bone = node->mTransformation;
        boneMat.push_back(current_bone);
        aiQuaterniont<float> rotation;
        aiVector3t<float> position;
        current_bone.DecomposeNoScaling(rotation, position);

        bonePos.push_back(position);
        boneRot.push_back(rotation);
        
        ProcessNode(node->mChildren[i], scene, commandList);
    }
    int i = 0;
}