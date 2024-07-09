#pragma once

namespace Renderer
{
	struct Vertex
	{
		std::array<float, 4> pos;
		std::array<float, 4> normal;
		std::array<float, 2> textureCoord;
	};

	struct Cluster
	{
		std::array<uint32_t, 8> light;
	};

	constexpr int VERTEX_SIZE_IN_BYTE = sizeof(Renderer::Vertex);
	constexpr int MAX_ELE_COUNT = 1 * 1024 * 1024 * 1024 / VERTEX_SIZE_IN_BYTE;
	constexpr int MAX_LIGHT_PER_TYPE = 32;
	constexpr int CLUSTER_X = 32;
	constexpr int CLUSTER_Y = 16;
	constexpr int CLUSTER_Z = 16;
}