#pragma once
#include "asset_loader.h"

namespace AssetLoader
{
	class ObjModelLoader final : public ModelAssetLoader
	{
	public:
		ObjModelLoader();

		~ObjModelLoader();

		std::optional<ECS::StaticMesh> LoadAssetFromFile(std::string InFileName) override;

	private:
	};

}