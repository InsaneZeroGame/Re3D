#include "window.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Window
{
	static void sOnResize(GLFWwindow* window, int InWidth, int InHeight)
	{
		gMainWindow->OnResize(InWidth, InHeight);
	}

	static void sCursorPos(GLFWwindow* window, double x, double y)
	{

	}
	static void sOnMouseEntered(GLFWwindow* window, int Entered)
	{
		if (Entered == GLFW_TRUE)
		{
			//spdlog::info("Entered");
		}
		else
		{
			//spdlog::info("Leave");
		}
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{

		}
	}

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{

	}

	BaseWindow::BaseWindow(int InWidth, int InHeight):
		mWidth(InWidth),
		mHeight(InHeight)
	{

	}

	BaseWindow::~BaseWindow()
	{

	}

	int BaseWindow::GetWidth()
	{
		return mWidth;
	}

	int BaseWindow::GetHeight()
	{
		return mHeight;
	}

	void BaseWindow::SetRenderFunc(std::function<void(float)> InFunc)
	{
		mRenderFunc = InFunc;
	}

    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


	Win32NavtiveWindow::Win32NavtiveWindow(int InWidth, int InHeight, HINSTANCE hInst, int nCmdShow):
	BaseWindow(InWidth,InHeight)
	{
        // Register class
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = &Win32NavtiveWindow::WndProc;
       // wcex.lpfnWndProc = std::bind(&Win32NavtiveWindow::WndProc, this, std::placeholders::_1, std::placeholders::_2,
       //                              std::placeholders::_3, std::placeholders::_4);
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInst;
        wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = "Re3D";
        wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION);
        Ensures(0 != RegisterClassEx(&wcex));

        // Create window
        RECT rc = { 0, 0, (LONG)InWidth, (LONG)InHeight };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        mNativeWindow = CreateWindow("Re3D", "Re3D", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInst, nullptr);

        Ensures(mNativeWindow != 0);

        //InitializeApplication(app);

        ShowWindow(mNativeWindow, nCmdShow /*SW_SHOWDEFAULT*/);
        mKeyboard = std::make_unique<DirectX::Keyboard>();
    }

	//--------------------------------------------------------------------------------------
    // Called every time the application receives a message
    //--------------------------------------------------------------------------------------
    //extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK Win32NavtiveWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_SIZE:
                //if (mRenderFunc) {
                //    mRenderFunc(0.0f);
                //}
                break;
            case WM_ACTIVATEAPP:
                DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
                //ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
                DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
                break;
            default:
                ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
                return DefWindowProc(hWnd, message, wParam, lParam);
        }

        return 0;
    }

    Win32NavtiveWindow::~Win32NavtiveWindow() {
    }

    void Win32NavtiveWindow::OnResize(int InWidth, int InHeight) {
    }

    void* Win32NavtiveWindow::GetNativeWindow() {
        return mNativeWindow;
    }

    void Win32NavtiveWindow::WindowLoop() 
	{
        do {
            MSG msg = {};
            bool done = false;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                auto kb = DirectX::Keyboard::Get().GetState();
                for (auto& Delegate: Delegates::KeyPressDelegate) 
				{
                    Delegate(kb);
                }
				
                if (msg.message == WM_QUIT)
                    done = true;
            }
            mRenderFunc(0.0f);
            if (done)
                break;
        } while (mRenderFunc);  // Returns false to quit loop
    }

    }  // namespace Window

Window::GlfwWindow::GlfwWindow(int InWidth, int InHeight):
	BaseWindow(InWidth,InHeight)
{
	/* Initialize the library */
	Ensures(glfwInit());

	/* Create a windowed mode window and its OpenGL context */
	mWindow = glfwCreateWindow(mWidth, mHeight, "Re3D", NULL, NULL);
	glfwMakeContextCurrent(mWindow);

	if (!mWindow)
	{
		glfwTerminate();
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwSetWindowSizeCallback(mWindow, &sOnResize);
	glfwSetCursorPosCallback(mWindow, &sCursorPos);
	glfwSetCursorEnterCallback(mWindow, &sOnMouseEntered);
	glfwSetScrollCallback(mWindow, &scroll_callback);
	glfwSwapInterval(0);
}

Window::GlfwWindow::~GlfwWindow()
{
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void Window::GlfwWindow::OnResize(int InWidth, int InHeight)
{
	std::cout << "Width : " << InWidth << " Height : " << InHeight << std::endl;
}

void* Window::GlfwWindow::GetNativeWindow()
{
	return glfwGetWin32Window(mWindow);
}

void Window::GlfwWindow::WindowLoop()
{
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(mWindow))
	{
		/* Render here */
		if (mRenderFunc)
		{
			mRenderFunc(0.0f);
		}
		/* Swap front and back buffers */
		glfwSwapBuffers(mWindow);

		/* Poll for and process events */
		glfwPollEvents();
	}
}

