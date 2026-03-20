using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

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
    }
}
