using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;

namespace QuantumEditor.Demo.Views
{
    /// <summary>
    /// Interaction logic for ShadowSceneInfoView.xaml
    /// </summary>
    public partial class ShadowSceneInfoView : UserControl
    {
        public ShadowSceneInfoView()
        {
            InitializeComponent();
        }

        private void OnShadowRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunShadowSceneRayTracingDX12(interop.Handle);
        }
    }
}
