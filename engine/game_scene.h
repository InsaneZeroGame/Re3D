#pragma once
namespace GAS
{
	class GameScene : public std::enable_shared_from_this<GameScene>
	{
	public:
		GameScene();

		virtual ~GameScene();

		std::vector<entt::entity> CreateEntitiesWithMesh(const std::string InMeshFilePath);

		entt::registry& GetRegistery();

		std::atomic_bool& IsSceneReady();

		using DelegateOnNewEntityAdded = std::function<void(std::shared_ptr<GameScene>, std::span<entt::entity>)>;
		inline static std::vector<DelegateOnNewEntityAdded> sOnNewEntityAdded;

	protected:
		entt::registry mRegistery;

		std::mutex mLoadMutex;
		
		std::atomic_bool mLoaded = false;
	};
}