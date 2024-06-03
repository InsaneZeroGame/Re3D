#pragma once

namespace GAS
{
	class GameScene
	{
	public:
		GameScene();

		virtual ~GameScene();

		void CreateEntitiesWithMesh(const std::string InMeshFilePath);

		entt::registry& GetRegistery();

		std::atomic_bool& IsSceneReady();

	protected:
		entt::registry mRegistery;

		std::mutex mLoadMutex;
		
		std::atomic_bool mLoaded = false;
	};
}