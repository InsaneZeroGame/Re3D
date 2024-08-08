#pragma once
#include "base_renderer.h"

namespace Renderer
{
	class DXRRenderer :public std::enable_shared_from_this<DXRRenderer>,public BaseRenderer
	{
	public:
		DXRRenderer();
		~DXRRenderer();


		void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight) override;


		void LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene) override;


		void Update(float delta) override;

	private:

	};
}