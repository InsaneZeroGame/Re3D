#pragma once

namespace Renderer
{
	class BaseRenderPass
	{
	public:
		BaseRenderPass();
		virtual ~BaseRenderPass();

	protected:
		void RenderObject();

	};
	
}