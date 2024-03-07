#include "stb_texture_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace AssetLoader
{
	std::optional<AssetLoader::Texture*> StbTextureAssetLoader::LoadTextureFromFile(std::string InFileName)
	{
		auto fileName = mModulePath.string() + "\\" + InFileName;

		if (!std::filesystem::exists(fileName))
		{
			return {};
		}
		Texture* newTexutre = new Texture;
		const auto& filePath = std::filesystem::path(fileName);
		const auto& fileExtension = filePath.extension();
		stbi_uc* data = stbi_load(filePath.string().c_str(), &newTexutre->mWidth, &newTexutre->mHeight, &newTexutre->mComponent, 4);
		if (!data)
		{
			return {};
		}
		newTexutre->mdata = new uint8_t[newTexutre->mWidth * newTexutre->mHeight * newTexutre->mComponent];
		memcpy(newTexutre->mdata, data, newTexutre->mWidth * newTexutre->mHeight * newTexutre->mComponent);
		return newTexutre;
	}

}

