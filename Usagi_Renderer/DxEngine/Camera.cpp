#include "pch.h"
#include "Camera.h"
#include "MathHelper.h"
#include "Events.h"

Camera::Camera(float aspectRatio)
{
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Camera::Update(const UpdateEventArgs& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

XMFLOAT3 Camera::GetPosition() const
{
	return mEyePos;
}

XMFLOAT4X4 Camera::GetViewMat() const
{
	return mView;
}

XMFLOAT4X4 Camera::GetProjMat() const
{
	return mProj;
}
