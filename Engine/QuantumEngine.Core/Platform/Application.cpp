#include "Application.h"
#include "GraphicWindow.h"

ATOM QuantumEngine::Platform::Application::winClass = 0;
HINSTANCE QuantumEngine::Platform::Application::m_app_instance = 0;

void QuantumEngine::Platform::Application::CreateApplication(HINSTANCE hInstance)
{
	m_app_instance = hInstance;
    CreateWindowClass();
}

ref<QuantumEngine::Platform::GraphicWindow> QuantumEngine::Platform::Application::CreateGraphicWindow(const WindowProperties& properties)
{
	return std::make_shared<GraphicWindow>(properties, winClass);
}

void QuantumEngine::Platform::Application::CreateWindowClass()
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(wcex); //allocation for this window class
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = &OnWindowMessage; // Function pointer for handling messages
    wcex.cbClsExtra = 0; //Allocate additional space for class
    wcex.cbWndExtra = sizeof(GraphicWindow*); //Allocate additional space for each window for storing pointer to created window. Used for getting window in nessage handler
    wcex.hInstance = m_app_instance;
    wcex.hIcon = LoadCursorW(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"WIN_CLASS";
    wcex.hIconSm = LoadCursorW(nullptr, IDI_APPLICATION);
    winClass = RegisterClassExW(&wcex);
}

LRESULT QuantumEngine::Platform::Application::OnWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GraphicWindow* window = reinterpret_cast<GraphicWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE: {
        GraphicWindow* window1 = reinterpret_cast<GraphicWindow*>(((CREATESTRUCTA*)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window1);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
