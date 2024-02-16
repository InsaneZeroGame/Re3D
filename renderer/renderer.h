#pragma once

namespace Renderer
{
	class BaseRenderer
	{
	public:
		BaseRenderer();
		virtual ~BaseRenderer();
		void SetTargetWindow(HWND InWindow, int InWidth, int InHeight);
		void Update(float delta);
	private:
		std::unique_ptr<class DeviceManager> mDeviceManager;
	};
}