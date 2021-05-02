using ExtProject.Domain;
using System;
using System.Reflection;
using System.Windows.Forms;

[assembly: ObfuscateAssemblyAttribute(true)]
namespace ExtProject
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new UAV());
        }
    }
}
