#pragma once

#include "./CommonWin.h"
#include "../BasicTypes.h"

namespace QuantumEngine::Platform {
	struct WindowProperties;
	class GraphicWindow;

	/// <summary>
	/// class wrapper for application variables. It is a singleton class
	/// </summary>
	class Application {
	public:
		static void CreateApplication(HINSTANCE hInstance); //Creates singleton for apploication 
		static ref<GraphicWindow> CreateGraphicWindow(const WindowProperties& properties); // Creates new window object with properties
	private:
		static void CreateWindowClass();

		static HINSTANCE m_app_instance;

		static ATOM winClass;
		static LRESULT CALLBACK OnWindowMessage(HWND, UINT, WPARAM, LPARAM);
	};
}