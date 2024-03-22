#include "asset_loader.h"
#include "stb_texture_loader.h"
#include "obj_model_loader.h"

namespace AssetLoader
{


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
	gObjModelLoader = new ObjModelLoader;
}

void AssetLoader::DestroyAssetLoader()
{
	if (gStbTextureLoader)
	{
		delete gStbTextureLoader;
		gStbTextureLoader = nullptr;
	}
	if (gObjModelLoader)
	{
		delete gObjModelLoader;
		gObjModelLoader = nullptr;
	}
}
