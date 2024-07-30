#include "engine.h"
#include "logger.h"
#include "asset_loader.h"


engine::GameEngine::GameEngine()
{

	InitLog();
	AssetLoader::InitAssetLoader();
}

engine::GameEngine::~GameEngine()
{

}

void engine::InitGameEngine()
{
	gGameEngine = new GameEngine;
}
