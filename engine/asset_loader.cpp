#include "asset_loader.h"
#include "stb_texture_loader.h"

namespace AssetLoader
{
	TextureAssetLoader* gStbTextureLoader = nullptr;

	BaseAssetLoader::BaseAssetLoader() :mModulePath("")
	{
		char moduleName[_MAX_PATH] = {};
		GetModuleFileNameA(nullptr, moduleName, _MAX_PATH);
		std::filesystem::path modulePath(moduleName);
		mModulePath = modulePath.parent_path();
	}

}

AssetLoader::TextureAssetLoader::TextureAssetLoader():BaseAssetLoader()
{

}

AssetLoader::TextureAssetLoader::~TextureAssetLoader()
{

}

void AssetLoader::InitAssetLoader()
{
	gStbTextureLoader = new StbTextureAssetLoader;
}

void AssetLoader::DestroyAssetLoader()
{
	if (gStbTextureLoader)
	{
		delete gStbTextureLoader;
		gStbTextureLoader = nullptr;
	}
}
