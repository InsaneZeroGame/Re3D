#include <iostream>
#include "window.h"

int main() 
{
	std::cout << "Hello Re3D" << std::endl;
	Window::gMainWindow = new Window::BaseWindow(800, 600);
	return 0;
}