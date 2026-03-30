using System.Runtime.InteropServices;

namespace QuantumEditor.Demo
{
    enum GraphicAPI
    {
        DIRECTX_12,
        VULKAN,
    }

    enum RenderMode
    {
        HYBRID,
        RAY_TRACING,
    }

    internal static partial class DemoNativeAPI
    {
        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Simple_Scene")]
        public static extern bool RunSimpleScene(IntPtr handle, GraphicAPI graphicAPI, RenderMode renderMode);


        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Reflection_Scene")]
        public static extern bool RunReflectionScene(IntPtr handle, GraphicAPI graphicAPI, RenderMode renderMode);

       
        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Shadow_Scene_RayTracing_DX12")]
        public static extern bool RunShadowSceneRayTracingDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Refraction_Scene_RayTracing_DX12")]
        public static extern bool RunRefractionSceneRayTracingDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Complete_Scene_RayTracing_DX12")]
        public static extern bool RunCompleteSceneRayTracingDX12(IntPtr handle);
    }
}
