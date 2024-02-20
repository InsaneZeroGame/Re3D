#include "camera.h"

Gameplay::BaseCamera::BaseCamera()
{
}

Gameplay::BaseCamera::~BaseCamera()
{

}

void Gameplay::BaseCamera::LookAt(SimpleMath::Vector3 InEye, SimpleMath::Vector3 InCenter, SimpleMath::Vector3 InUp)
{
	 mView = SimpleMath::Matrix::CreateLookAt(InEye, InCenter, InUp);
}

const DirectX::SimpleMath::Matrix& Gameplay::BaseCamera::GetPrj()
{
	return mPrj.Transpose();
}

const DirectX::SimpleMath::Matrix& Gameplay::BaseCamera::GetView()
{
	return mView.Transpose();
}

const DirectX::SimpleMath::Matrix& Gameplay::BaseCamera::GetPrjView()
{
	return (mView * mPrj).Transpose();
}

Gameplay::PerspectCamera::PerspectCamera(float InWidth, float InHeight, float InNear, float InFar):
	BaseCamera(),
	mWidth(InWidth),
	mHeight(InHeight),
	mNear(InNear),
	mFar(InFar)
{
	//mPrj = XMMatrixPerspectiveLH(mWidth,mHeight,mNear,mFar);
	//mPrj = XMMatrixPerspectiveFovLH(0.785398185,mWidth/ mHeight, mNear, mFar);
	float aspectRatio = (float)InWidth / (float)InHeight;
	float fovAngleY = 90.0 * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
	{
		fovAngleY /= aspectRatio;
	}
	mPrj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(fovAngleY, aspectRatio, InNear, InFar);
}

Gameplay::PerspectCamera::PerspectCamera()
{

}

Gameplay::PerspectCamera::~PerspectCamera()
{

}
