#pragma once
namespace Delegates
{
	inline std::vector<std::function<void(int key, int scancode, int action, int mods)>> KeyPressDelegate = {};
}

