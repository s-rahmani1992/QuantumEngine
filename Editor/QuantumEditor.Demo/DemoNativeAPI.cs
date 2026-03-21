using System.Runtime.InteropServices;

namespace QuantumEditor.Demo
{
    internal static partial class DemoNativeAPI
    {
        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Light_Scene_Rasterization_DX12")]
        public static extern bool RunSimpleSceneRasterizationDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Light_Scene_RayTracing_DX12")]
        public static extern bool RunSimpleSceneRayTracingDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Reflection_Scene_Hybrid_DX12")]
        public static extern bool RunReflectionSceneHybridDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Reflection_Scene_RayTracing_DX12")]
        public static extern bool RunReflectionSceneRayTracingDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Shadow_Scene_RayTracing_DX12")]
        public static extern bool RunShadowSceneRayTracingDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Refraction_Scene_RayTracing_DX12")]
        public static extern bool RunRefractionSceneRayTracingDX12(IntPtr handle);
    }
}
