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
}