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
            DemoNativeAPI.RunReflectionSceneHybridDX12(interop.Handle);
        }

        private void OnReflectionRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunReflectionSceneRayTracingDX12(interop.Handle);
        }
    }
}
