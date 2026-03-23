using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;

namespace QuantumEditor.Demo.Views
{
    /// <summary>
    /// Interaction logic for SimpleSceneInfoView.xaml
    /// </summary>
    public partial class SimpleSceneInfoView : UserControl
    {
        public SimpleSceneInfoView()
        {
            InitializeComponent();
        }

        private void OnSimpleRasterizationDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleSceneRasterizationDX12(interop.Handle);
        }

        private void OnSimpleRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleSceneRayTracingDX12(interop.Handle);
        }

        private void OnSimpleRasterizationVK(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleSceneRasterizationVK(interop.Handle);
        }
    }
}
