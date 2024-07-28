#include "asset_loader.h"

namespace AssetLoader
{
	class StbTextureAssetLoader final : public TextureAssetLoader
	{
	public:
		StbTextureAssetLoader() {};
		~StbTextureAssetLoader() {};
		std::optional<TextureData*> LoadTextureFromFile(std::string_view InFileName) override;

		std::optional<TextureData*> stbLoadTexture(const std::filesystem::path& filePath);

		std::optional<TextureData*> ddsLoadTexture(const std::filesystem::path& filePath);

	private:

	};

	
}

