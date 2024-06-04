#include "asset_loader.h"

namespace AssetLoader
{
	class StbTextureAssetLoader final : public TextureAssetLoader
	{
	public:
		StbTextureAssetLoader() {};
		~StbTextureAssetLoader() {};
		std::optional<TextureData*> LoadTextureFromFile(std::string_view InFileName) override;
	private:

	};

	
}

