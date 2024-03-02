#pragma once

namespace Renderer
{
	struct Vertex
	{
		std::array<float, 4> pos;
		std::array<float, 4> normal;
		std::array<float, 4> color;
		std::array<float, 2> textureCoord;
	};

	struct Light
	{
		std::array<float, 4> color;
		std::array<float, 4> pos;
		std::array<float, 4> radius_attenu;
	};

	struct Cluster
	{	
		std::array<uint32_t,8> light;
		
	};
}