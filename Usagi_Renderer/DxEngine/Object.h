#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <wrl.h>
#include <memory>

using namespace Microsoft::WRL;
using namespace DirectX;
class CommandList;
class Model;

class Object
{
public:
	Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 mAlbedo, XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));

public:
	virtual void Update(float dt);
	virtual void Draw(CommandList& commandList);
	XMMATRIX GetWorldMat() const;
	void SetPosition(XMVECTOR newPos);
	void SetScale(XMVECTOR newScale);
	void SetAlbedo(XMFLOAT3 newAlbedo);
	void SetModel(std::shared_ptr<Model> model);

protected:
	void SetWorldMatrix(CommandList& commandList);

	std::shared_ptr<Model> mModel = nullptr;
	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
	XMFLOAT3 mAlbedo;
};

