// QuantumEngine.Test.cpp : Defines the entry point for the application.
//

#include "QuantumEngine.Test.h"
#include <DemoAPI.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Run_Complete_Scene_RayTracing_DX12(nullptr);

    return 0;
}
