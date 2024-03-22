#pragma once

namespace GAS
{
	class GameScene
	{
	public:
		GameScene();

		virtual ~GameScene();

		entt::entity CreateEntityWithMesh(const std::string InMeshFilePath);

		entt::registry& GetRegistery();

	protected:
		entt::registry mRegistery;
	};
}