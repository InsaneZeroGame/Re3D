#include "stb_texture_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


std::string str_tolower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::tolower(c); } 
	);
	return s;
}

namespace AssetLoader
{
	std::optional<AssetLoader::TextureData*> StbTextureAssetLoader::LoadTextureFromFile(std::string_view InFileName)
	{
		std::string fileName = InFileName.data();

		if (!std::filesystem::exists(fileName))
		{
			fileName = mModulePath.string() + "\\" + InFileName.data();
			if (!std::filesystem::exists(fileName))
			{
				return {};
			}
		}
		const auto& filePath = std::filesystem::path(fileName);
		const auto& fileExtension = str_tolower(filePath.extension().string());

		if (fileExtension == ".png")
		{
			return stbLoadTexture(filePath);
		}
		else
		{
			return ddsLoadTexture(filePath);
		}


	}

	std::optional<TextureData*> StbTextureAssetLoader::stbLoadTexture(const std::filesystem::path& filePath)
	{
		TextureData* newTexutre = new TextureData;
		stbi_uc* data = stbi_load(filePath.string().c_str(), &newTexutre->mWidth, &newTexutre->mHeight, &newTexutre->mComponent, 4);
		newTexutre->mComponent = 4;
		if (!data)
		{
			return {};
		}
		newTexutre->mdata = new uint8_t[newTexutre->mWidth * newTexutre->mHeight * newTexutre->mComponent];
		memcpy(newTexutre->mdata, data, newTexutre->mWidth * newTexutre->mHeight * newTexutre->mComponent);
		return newTexutre;
	}

	std::optional<TextureData*> StbTextureAssetLoader::ddsLoadTexture(const std::filesystem::path& filePath)
	{
		//By pass texture loading to renderer.
		TextureData* newTexutre = new TextureData;
		newTexutre->mFilePath = filePath.string();
		return newTexutre;
	}

}

