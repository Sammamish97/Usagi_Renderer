#include "pch.h"
#include "Object.h"
#include "Model.h"
#include "CommandList.h"

Object::Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 albedo, XMFLOAT3 scale)
	:mModel(model), mPosition(position), mAlbedo(albedo), mScale(scale)
{
}

void Object::SetWorldMatrix(CommandList& commandList)
{
	XMMATRIX worldMat = GetWorldMat();
	worldMat = XMMatrixTranspose(worldMat);
	commandList.SetGraphics32BitConstants(0, worldMat);
}

void Object::Update(float dt)
{
}

XMMATRIX Object::GetWorldMat() const
{
	XMMATRIX translationMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	XMMATRIX scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

	return XMMatrixMultiply(scaleMat, translationMat);
}

void Object::Draw(CommandList& commandList)
{
	SetWorldMatrix(commandList);
	for (auto& mesh : mModel->mMeshes)
	{
		mesh.Draw(commandList);
	}
}

void Object::SetPosition(XMVECTOR newPos)
{
	XMStoreFloat3(&mPosition, newPos);
}

void Object::SetPosition(XMFLOAT3 newPos)
{
	mPosition = newPos;
}

void Object::SetScale(XMVECTOR newScale)
{
	XMStoreFloat3(&mScale, newScale);
}

void Object::SetAlbedo(XMFLOAT3 newAlbedo)
{
	mAlbedo = newAlbedo;
}

void Object::SetModel(std::shared_ptr<Model> model)
{
	mModel = model;
}