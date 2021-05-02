using System;
using System.Runtime.InteropServices;

namespace ExtProject.MemoryManagement
{
    public class ProcessMethods
    {
        private int pID;

        public ProcessMethods()
        {
            pID = GetProcessIDByName("EscapeFromTarkov.exe");
        }
        public ulong ReadMemory(ulong lpBaseAddress)
        {
            return ProcessMethods.NativeMethods.read(13392, lpBaseAddress);
        }

        public void ReadBytesFromMemory(ulong lpBaseAddress, byte[] buffer, int size)
        {
            ProcessMethods.NativeMethods.read_memory(pID, lpBaseAddress, buffer, size);
        }

        public void WriteMemory(ulong lpBaseAddress, byte[] buffer)
        {
            ProcessMethods.NativeMethods.write(pID, lpBaseAddress, buffer);
        }

        public ulong GetModuleBaseAddress(String modName)
        {
            return ProcessMethods.NativeMethods.GetModuleBaseAddress(pID, modName);
        }

        public int GetProcessIDByName(String procName)
        {
            return ProcessMethods.NativeMethods.GrabProcessByName(procName);
        }

        public bool Init()
        {
            return NativeMethods.initialize();
        }

        private static class NativeMethods
        {
            [DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            internal static extern ulong read(int hProcess, ulong lpBaseAddress);

            [DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            internal static extern ulong read_memory(int hProcess, ulong lpBaseAddress, byte[] buffer, int size);

            [DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            internal static extern void write(int hProcess, ulong lpBaseAddress, byte[] buffer);

            [DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            public static extern ulong GetModuleBaseAddress(int procHandle, String modName);

            [DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            public static extern int GrabProcessByName(String procName);

            [DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool initialize();
        }
    }
}
