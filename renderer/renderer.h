#pragma once

namespace Renderer
{
	class BaseRenderer
	{
	public:
		BaseRenderer();
		virtual ~BaseRenderer();
		void SetTargetWindow(HWND InWindow, int InWidth, int InHeight);
	private:
		std::unique_ptr<class DeviceManager> mDeviceManager;
	};
}