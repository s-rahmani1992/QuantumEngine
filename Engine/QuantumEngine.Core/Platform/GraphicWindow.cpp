#include "GraphicWindow.h"

QuantumEngine::Platform::GraphicWindow::GraphicWindow(const WindowProperties& properties, const ATOM& winClass)
    : m_width(properties.width), m_height(properties.height), m_title(properties.title)
{
    m_handle = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW,
        (LPCWSTR)winClass, m_title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, m_width, m_height, nullptr, nullptr, GetModuleHandleW(nullptr), this);
}

bool QuantumEngine::Platform::GraphicWindow::Update()
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}
