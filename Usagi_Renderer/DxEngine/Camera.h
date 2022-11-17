#pragma once
#include <DirectXMath.h>
#include "MathHelper.h"

using namespace DirectX;

class UpdateEventArgs;
struct Camera
{
public:
	Camera(float aspectRatio = 45.f);
	void Update(const UpdateEventArgs& gt);
	XMFLOAT3 GetPosition() const;
	XMFLOAT4X4 GetViewMat() const;
	XMFLOAT4X4 GetProjMat() const;

	float mTheta = 1.5f * XM_PI;
	float mPhi = 0.2f * XM_PI;
	float mRadius = 5.0f;

private:
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};


