using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace QuantumEditor.Demo
{
    internal static partial class DemoNativeAPI
    {
        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Light_Scene_Rasterization_DX12")]
        public static extern bool RunSimpleSceneRasterizationDX12(IntPtr handle);

        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Light_Scene_RayTracing_DX12")]
        public static extern bool RunSimpleSceneRayTracingDX12(IntPtr handle);
    }
}
