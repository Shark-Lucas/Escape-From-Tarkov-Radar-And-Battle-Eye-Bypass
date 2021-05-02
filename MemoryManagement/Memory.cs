using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace ExtProject.MemoryManagement
{
    static class Memory
    {
        public static ProcessService processService = null;
        public static ProcessMethods processMethods = new ProcessMethods();

        public static ulong ImageBase()
        {
            if (!currentlyRunning())
            {
                return 0x0;
            }

            ulong baseAddress = processMethods.GetModuleBaseAddress("UnityPlayer.dll");

            return baseAddress;
        }

        public static byte[] ReadBytes(ulong address, Int32 bufferSize)
        {
            if (!currentlyRunning())
            {
                return null;
            }

            byte[] buffer = new byte[bufferSize];
            processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);

            return buffer;
        }

        public static T Read<T>(ulong address, int offset)
        {
            try
            {
                int size = Marshal.SizeOf(typeof(T));
                byte[] buffer = new byte[offset];
                processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);
                GCHandle handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
                var pinnedAdd = handle.AddrOfPinnedObject();
                T data = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
                handle.Free();
                return data;
            }
            catch (Exception)
            {
                return default(T);
            }
        }
        public static T ReadGenericType<T>(ulong address, Int32 bufferSize)
        {
            if (!currentlyRunning())
            {
                return (T)Convert.ChangeType("0", typeof(T)); ;
            }

            byte[] buffer = new byte[bufferSize];
            processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);

            switch (GetGenericType(new Dictionary<int, T>()))
            {
                case "single":
                    return (T)Convert.ChangeType(BitConverter.ToSingle(buffer, 0), typeof(T));
                case "string":
                    return (T)Convert.ChangeType(Encoding.UTF8.GetString(buffer), typeof(T));
                case "int32":
                    return (T)Convert.ChangeType(BitConverter.ToInt32(buffer, 0), typeof(T));
                case "int64":
                    return (T)Convert.ChangeType(BitConverter.ToInt64(buffer, 0), typeof(T));
                case "byte":
                    return (T)Convert.ChangeType(buffer[0], typeof(T));
                case "byte[]":
                    return (T)Convert.ChangeType(buffer, typeof(T));
                default:
                    MessageBox.Show("Default" + Environment.NewLine + GetGenericType(new Dictionary<int, T>()));
                    return (T)Convert.ChangeType("0", typeof(T));
            }
        }

        public static String ReadUnicodeString(ulong address, Int32 bufferSize)
        {
            byte[] buffer = new byte[bufferSize];
            processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);
            return Encoding.Unicode.GetString(buffer);
        }

        public static T ReadGenericType<T>(ulong address, Int32 bufferSize, bool addBase = false)
        {
            if (!currentlyRunning())
            {
                return (T)Convert.ChangeType("0", typeof(T)); ;
            }

            byte[] buffer = new byte[bufferSize];


            if (addBase)
            {
                processMethods.ReadBytesFromMemory(Memory.ImageBase() + address, buffer, buffer.Length);
            }
            else
            {
                processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);
            }
            switch (GetGenericType(new Dictionary<int, T>()))
            {
                case "single":
                    return (T)Convert.ChangeType(BitConverter.ToSingle(buffer, 0), typeof(T));
                case "string":
                    return (T)Convert.ChangeType(Encoding.UTF8.GetString(buffer), typeof(T));
                case "int32":
                    return (T)Convert.ChangeType(BitConverter.ToInt32(buffer, 0), typeof(T));
                case "int64":
                    return (T)Convert.ChangeType(BitConverter.ToInt64(buffer, 0), typeof(T));
                case "byte":
                    return (T)Convert.ChangeType(buffer[0], typeof(T));
                case "byte[]":
                    return (T)Convert.ChangeType(buffer, typeof(T));
                default:
                    MessageBox.Show("Default" + Environment.NewLine + GetGenericType(new Dictionary<int, T>()));
                    return (T)Convert.ChangeType("0", typeof(T));
            }
        }

        public static T Read<T>(ulong address, bool addBase = false, int customSize = -1)
        {
            try
            {
                int size = customSize == -1 ? Marshal.SizeOf(typeof(T)) : customSize;
                byte[] buffer = new byte[size];

                if (addBase)
                {
                    processMethods.ReadBytesFromMemory(ImageBase() + address, buffer, buffer.Length);
                }
                else
                {
                    processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);
                }

                GCHandle handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
                T data = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));

                handle.Free();

                var pointerToHex = BitConverter.ToString(buffer).Replace("-", "");

                return data;
            }
            catch (Exception)
            {
                return default(T);
            }
        }

        public static ulong ReadPtr(ulong address, bool addBase = false, int customSize = -1)
        {
            try
            {
                int size = customSize == -1 ? Marshal.SizeOf(typeof(ulong)) : customSize;
                byte[] buffer = new byte[size];

                if (addBase)
                {
                    processMethods.ReadBytesFromMemory(ImageBase() + address, buffer, buffer.Length);
                }
                else
                {
                    processMethods.ReadBytesFromMemory(address, buffer, buffer.Length);
                }

                var pointerToHex = BitConverter.ToString(buffer).Replace("-", "");
                var HexToInt = Convert.ToUInt64(pointerToHex, 16);

                return HexToInt;
            }
            catch (Exception)
            {
                return default;
            }

        }

        public static void WriteNops(ulong address, int amount)
        {
            byte[] nops = new byte[amount];
            for (int i = 0; i < amount; i++)
            {
                nops[i] = 144;
            }
            WriteBytes(address, nops);
        }
        public static void WriteBytes(ulong address, dynamic val)
        {
            if (!currentlyRunning())
            {
                return;
            }

            byte[] value = null;

            try
            {
                value = BitConverter.GetBytes(val);
            }
            catch
            {
                value = val;
            }
            processMethods.WriteMemory(address, value);
        }
        public static void WriteInt(ulong address, int value)
        {
            if (!currentlyRunning())
            {
                return;
            }

            byte[] buffer = BitConverter.GetBytes(value);
            processMethods.WriteMemory(address, buffer);
        }
        public static void WriteFloat(ulong address, float value)
        {
            if (!currentlyRunning())
            {
                return;
            }

            byte[] buffer = BitConverter.GetBytes(value);
            processMethods.WriteMemory(address, buffer);
        }
        public static void WriteShort(ulong address, short value)
        {
            if (!currentlyRunning())
            {
                return;
            }

            byte[] buffer = BitConverter.GetBytes(value);
            processMethods.WriteMemory(address, buffer);
        }
        public static void WriteByte(ulong address, byte value)
        {
            if (!currentlyRunning())
            {
                return;
            }

            byte[] buffer = BitConverter.GetBytes(value);
            processMethods.WriteMemory(address, buffer);
        }
        public static string GetGenericType<T>(Dictionary<int, T> list)
        {
            Type type = list.GetType().GetProperty("Item").PropertyType;
            string typeName = type.Name.ToLower();
            return typeName;
        }
        public static bool currentlyRunning()
        {
            try
            {
                if (processService == null)
                {
                    processService = new ProcessService();
                }
                return true;
            }
            catch
            {
                return false;
            }
        }
        public static ulong ptrChain(ulong address, params uint[] ptrChainOffsets)
        {
            var curAddr = address;

            foreach (var offset in ptrChainOffsets)
            {
                curAddr = Read<UInt64>(curAddr + offset);

                if (curAddr == 0)
                    return 0;
            }

            return curAddr;
        }
    }
}
