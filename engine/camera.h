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
		const SimpleMath::Matrix& GetPrj();
		const SimpleMath::Matrix& GetView();
		const SimpleMath::Matrix& GetPrjView();
		virtual void KeyDown(int key, int scancode, int action, int mods);
	protected:
		void Forward(float InSpeed);
		void Right(float InSpeed);
		void Yaw(float InSpeed);
		SimpleMath::Vector3 mEye;
		SimpleMath::Vector3 mCenter;
		SimpleMath::Vector3 mUp;
		SimpleMath::Matrix mPrj;
		SimpleMath::Matrix mView;
	};

	class PerspectCamera final : public BaseCamera
	{
	public:
		PerspectCamera();
		PerspectCamera(float InWidth,float InHeight,float InNear,float InFar);
		~PerspectCamera();

	private:
		float mWidth;
		float mHeight;
		float mNear;
		float mFar;
	};
}