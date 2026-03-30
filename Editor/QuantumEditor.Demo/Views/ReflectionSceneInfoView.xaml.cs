using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;

namespace QuantumEditor.Demo.Views
{
    /// <summary>
    /// Interaction logic for ReflectionSceneInfoView.xaml
    /// </summary>
    public partial class ReflectionSceneInfoView : UserControl
    {
        public ReflectionSceneInfoView()
        {
            InitializeComponent();
        }

        private void OnReflectionHybridDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunReflectionScene(interop.Handle, GraphicAPI.DIRECTX_12, RenderMode.HYBRID);
        }

        private void OnReflectionRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunReflectionScene(interop.Handle, GraphicAPI.DIRECTX_12, RenderMode.RAY_TRACING);
        }

        private void OnReflectionRayTracingVulkan(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunReflectionScene(interop.Handle, GraphicAPI.VULKAN, RenderMode.RAY_TRACING);
        }
    }
}
