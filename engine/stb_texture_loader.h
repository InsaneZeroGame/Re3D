#include "asset_loader.h"

namespace AssetLoader
{
	class StbTextureAssetLoader final : public TextureAssetLoader
	{
	public:
		StbTextureAssetLoader() {};
		~StbTextureAssetLoader() {};
		std::optional<Texture*> LoadTextureFromFile(std::string InFileName) override;
	private:

	};

	
}

