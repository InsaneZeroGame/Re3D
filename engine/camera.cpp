#include "camera.h"
#include "delegates.h"
#include <GLFW/glfw3.h>


Gameplay::BaseCamera::BaseCamera()
{
	Delegates::KeyPressDelegate.push_back(std::bind(&BaseCamera::KeyDown, this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3,
		std::placeholders::_4
	));
}

Gameplay::BaseCamera::~BaseCamera()
{

}

void Gameplay::BaseCamera::LookAt(SimpleMath::Vector3 InEye, SimpleMath::Vector3 InCenter, SimpleMath::Vector3 InUp)
{
	mEye = InEye;
	mCenter = InCenter;
	mUp = InUp;
	mView = XMMatrixLookAtLH(mEye,mCenter,mUp);
}

const DirectX::SimpleMath::Matrix Gameplay::BaseCamera::GetPrj(bool UploadToGpu)
{
	return UploadToGpu ? mViewToClip.Transpose() : mViewToClip;
}

const DirectX::SimpleMath::Matrix Gameplay::BaseCamera::GetView(bool UploadToGpu)
{
	return UploadToGpu ? mView.Transpose(): mView;
}

const DirectX::SimpleMath::Matrix Gameplay::BaseCamera::GetNormalMatrix(bool UploadToGpu /*= true*/)
{
	return UploadToGpu ? mView.Invert() : mView.Invert().Transpose();
}

const DirectX::SimpleMath::Matrix Gameplay::BaseCamera::GetPrjView(bool UploadToGpu)
{
	return UploadToGpu ? (mView * mViewToClip).Transpose() : mView * mViewToClip;
}

void Gameplay::BaseCamera::KeyDown(int key, int scancode, int action, int mods)
{
	
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			Forward(0.1f);
			break;
		case GLFW_KEY_S:
			Forward(-0.1f);
			break;
		case GLFW_KEY_A:
			Right(0.1f);
			break;
		case GLFW_KEY_D:
			Right(-0.1f);
			break;
		case GLFW_KEY_Q:
			Yaw(-0.1f);
			break;
		case GLFW_KEY_E:
			Yaw(0.1f);
			break;
		default:
			break;
		}
	}
}

DirectX::SimpleMath::Matrix Gameplay::BaseCamera::GetClipToView(bool UploadToGpu)
{
	return UploadToGpu ? mClipToView.Transpose(): mClipToView;
}

void Gameplay::BaseCamera::Forward(float InSpeed)
{
	auto forwardVec = mCenter - mEye;
	auto forwardStep = forwardVec * InSpeed;
	SimpleMath::Vector3::Transform(mEye, SimpleMath::Matrix::CreateTranslation(forwardStep),mEye);
	SimpleMath::Vector3::Transform(mCenter, SimpleMath::Matrix::CreateTranslation(forwardStep), mCenter);

	LookAt(mEye, mCenter, mUp);
}

void Gameplay::BaseCamera::Right(float InSpeed)
{
	SimpleMath::Vector3 forwardVec = mCenter - mEye;
	SimpleMath::Vector3 rightVec = forwardVec.Cross(mUp);
	rightVec.Normalize();
	auto rightStep = rightVec * InSpeed;
	SimpleMath::Vector3::Transform(mEye, SimpleMath::Matrix::CreateTranslation(rightStep), mEye);
	SimpleMath::Vector3::Transform(mCenter, SimpleMath::Matrix::CreateTranslation(rightStep), mCenter);
	LookAt(mEye, mCenter, mUp);
}

void Gameplay::BaseCamera::Yaw(float InSpeed)
{
	SimpleMath::Vector3 forwardVec = mCenter - mEye;
	SimpleMath::Vector3 newForwardVec;
	SimpleMath::Vector3::Transform(forwardVec, SimpleMath::Matrix::CreateRotationY(InSpeed), newForwardVec);
	mCenter = mEye + newForwardVec;
	LookAt(mEye, mCenter, mUp);
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
	mViewToClip = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, InNear, InFar);
	mClipToView = mViewToClip.Invert();
}

Gameplay::PerspectCamera::PerspectCamera(float InWidth, float InHeight, float InNear):
	BaseCamera(),
	mWidth(InWidth),
	mHeight(InHeight),
	mNear(InNear),
	mFar(125.0)
{
	float aspectRatio = (float)InWidth / (float)InHeight;
	float fovAngleY = 90.0 * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
	{
		fovAngleY /= aspectRatio;
	}
	fovAngleY *= 0.5;
	mViewToClip = SimpleMath::Matrix(
		SimpleMath::Vector4(1.0f / tan(fovAngleY), 0.0f, 0.0f, 0.0f),
		SimpleMath::Vector4(0.0f, InWidth / tan(fovAngleY) / InHeight, 0.0f, 0.0f),
		SimpleMath::Vector4(0.0f, 0.0f, 0.0f, 1.0f),
		SimpleMath::Vector4(0.0f, 0.0f, InNear, 0.0f)
	);
	mClipToView = mViewToClip.Invert();
}

Gameplay::PerspectCamera::PerspectCamera()
{

}

Gameplay::PerspectCamera::~PerspectCamera()
{

}

float Gameplay::PerspectCamera::GetNear()
{
	return mNear;
}

float Gameplay::PerspectCamera::GetFar()
{
	return mFar;
}
