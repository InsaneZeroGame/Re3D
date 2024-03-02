#pragma once

namespace Gameplay
{
	using namespace DirectX;
	class BaseCamera
	{
	public:
		BaseCamera();
		virtual ~BaseCamera();
		virtual void LookAt(SimpleMath::Vector3 InEye, SimpleMath::Vector3 InCenter, SimpleMath::Vector3 InUp);
		const SimpleMath::Matrix GetPrj(bool UploadToGpu = true);
		const SimpleMath::Matrix GetView(bool UploadToGpu = true);
		const SimpleMath::Matrix GetNormalMatrix(bool UploadToGpu = true);
		const SimpleMath::Matrix GetPrjView(bool UploadToGpu = true);
		virtual void KeyDown(int key, int scancode, int action, int mods);
		SimpleMath::Matrix GetClipToView(bool UploadToGpu = true);
	protected:
		void Forward(float InSpeed);
		void Right(float InSpeed);
		void Yaw(float InSpeed);
		SimpleMath::Vector3 mEye;
		SimpleMath::Vector3 mCenter;
		SimpleMath::Vector3 mUp;
		SimpleMath::Matrix mViewToClip;
		SimpleMath::Matrix mClipToView;
		SimpleMath::Matrix mView;
	};

	class PerspectCamera final : public BaseCamera
	{
	public:
		PerspectCamera();
		PerspectCamera(float InWidth,float InHeight,float InNear,float InFar);
		PerspectCamera(float InWidth, float InHeight, float InNear);
		~PerspectCamera();
		float GetNear();
		float GetFar();
	private:
		float mWidth;
		float mHeight;
		float mNear;
		float mFar;
		bool mReversedZ = true;
		bool mInfinite = true;
	};
}