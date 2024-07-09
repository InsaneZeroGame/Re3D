#pragma once

namespace Gameplay
{
	using namespace DirectX;
	class BaseCamera
	{
	public:
		BaseCamera(bool mMoveable = false);
		virtual ~BaseCamera();
		virtual void LookAt(SimpleMath::Vector3 InEye, SimpleMath::Vector3 InCenter, SimpleMath::Vector3 InUp);
		const SimpleMath::Matrix GetPrj(bool UploadToGpu = true);
		const SimpleMath::Matrix GetView(bool UploadToGpu = true);
		const SimpleMath::Matrix GetNormalMatrix(bool UploadToGpu = true);
		const SimpleMath::Matrix GetPrjView(bool UploadToGpu = true);
		//virtual void KeyDown(int key, int scancode, int action, int mods);
        virtual void KeyDown(Keyboard::State InState);
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
		bool mMoveable = false;
		float mSpeed = 50.0f;
	};

	class PerspectCamera final : public BaseCamera
	{
	public:
		PerspectCamera(bool mMoveable = false);
		PerspectCamera(float InWidth,float InHeight,float InNear,float InFar, bool mMoveable = false);
		PerspectCamera(float InWidth, float InHeight, float InNear, bool mMoveable = false);
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