#pragma once

#include "./CommonWin.h"
#include "../BasicTypes.h"
#include "../Rendering/GPUDeviceManager.h"

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
		inline static ref<Rendering::GPUDeviceManager> GetGPUDevice() { return m_instance.m_gpu_device; } 
		
		template<class T>
		static ref<Rendering::GPUDeviceManager> InitializeGraphicDevice() {
			m_instance.m_gpu_device = std::make_shared<T>();
			m_instance.m_gpu_device->Initialize();
			return m_instance.m_gpu_device;
		}

	private:
		void CreateWindowClass();
		static Application m_instance;

		HINSTANCE m_app_instance;
		ATOM winClass;
		ref<Rendering::GPUDeviceManager> m_gpu_device;

		static LRESULT CALLBACK OnWindowMessage(HWND, UINT, WPARAM, LPARAM);
	};
}