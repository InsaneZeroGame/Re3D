#pragma once
#include "graphics_common.h"

namespace AssetLoader
{
	struct Mesh
	{	
		std::vector<Renderer::Vertex> mVertices;
		std::vector<int> mIndices;
	};

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

	struct ModelAsset
	{	
		std::vector<Mesh> mMeshes;
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

		virtual std::optional<ModelAsset> LoadAssetFromFile(std::string InFileName) { return {}; };

	private:

	};

	class TextureAssetLoader: public BaseAssetLoader
	{
	public:
		TextureAssetLoader();

		virtual ~TextureAssetLoader();

		virtual std::optional<Texture*> LoadTextureFromFile(std::string InFileName) { return {}; };

	private:

	};

	extern TextureAssetLoader* gStbTextureLoader;

	void InitAssetLoader();
	void DestroyAssetLoader();
	
}





