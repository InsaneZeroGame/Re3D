#pragma once
#include "components.h"

namespace AssetLoader
{
	struct Texture
	{
		int mWidth;
		int mHeight;
		int mComponent;
		uint8_t* mdata;

		Texture():mdata(nullptr),mWidth(0),mHeight(0),mComponent(0) {};
		~Texture() 
		{
			if (mdata)
			{
				delete mdata;
				mdata = nullptr;
			}
		}
	};

	class BaseAssetLoader
	{
	public:
		BaseAssetLoader();
		virtual ~BaseAssetLoader() {};
	protected:
		std::filesystem::path mModulePath;

	};

	class ModelAssetLoader: public BaseAssetLoader
	{
	public:
		ModelAssetLoader() : BaseAssetLoader() {};

		virtual ~ModelAssetLoader() {};

		virtual std::vector<ECS::StaticMesh>& LoadAssetFromFile(std::string InFileName) { return mStaticMeshes; };

	protected:
        std::vector<ECS::StaticMesh> mStaticMeshes;

	};

	class TextureAssetLoader: public BaseAssetLoader
	{
	public:
		TextureAssetLoader();

		virtual ~TextureAssetLoader();

		virtual std::optional<Texture*> LoadTextureFromFile(std::string InFileName) { return {}; };

	private:

	};

	inline TextureAssetLoader* gStbTextureLoader;
	inline class ObjModelLoader* gObjModelLoader;
    inline class FbxLoader* gFbxModelLoader;

	void InitAssetLoader();
	void DestroyAssetLoader();
	
}





