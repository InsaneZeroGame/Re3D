#pragma once

namespace engine
{

	class GameEngine
	{
	public:
		GameEngine();
		virtual ~GameEngine();

	private:

	};


	void InitGameEngine();
	inline GameEngine* gGameEngine;

}