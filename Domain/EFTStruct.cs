using ExtProject.MemoryManagement;
using System;
using System.Collections.Generic;
using System.Linq;

namespace ExtProject.Domain
{
    // Unused
    class EFTStruct
    {
        public ulong _basePTR = 0;
        private Dictionary<string, Dictionary<uint, uint[]>> _structs = null;
        private ProcessMethods processMethods = new ProcessMethods();

        public EFTStruct(ulong basePTR, Dictionary<string, Dictionary<uint, uint[]>> structs)
        {
            _basePTR = basePTR;
            _structs = structs;
        }
        public ulong GetPointer(string s)
        {
            try
            {
                if (_structs.ContainsKey(s))
                {
                    Dictionary<uint, uint[]> structDetails = _structs[s];
                    uint bufferSize = structDetails.ElementAt(0).Key;
                    uint[] offsets = structDetails.ElementAt(0).Value;

                    ulong memoryLocation = Memory.ptrChain(_basePTR, offsets);

                    return memoryLocation;

                }
                return 0x0;
            }
            catch
            {
                return 0x0;
            }
        }
        public dynamic GetValue<T>(string s)
        {
            try
            {
                if (_structs.ContainsKey(s))
                {
                    Dictionary<uint, uint[]> structDetails = _structs[s];
                    uint bufferSize = structDetails.ElementAt(0).Key;
                    uint[] offsets = structDetails.ElementAt(0).Value;

                    ulong memoryLocation = Memory.ptrChain(_basePTR, offsets);
                    byte[] buffer = Memory.ReadBytes(memoryLocation, (int)bufferSize);

                    switch (Memory.GetGenericType(new Dictionary<int, T>()))
                    {
                        case "string":
                            return Memory.ReadGenericType<T>(memoryLocation, 32);

                        default:
                            return processMethods.ReadMemory(memoryLocation);
                    }


                }
                return (T)Convert.ChangeType("0", typeof(T));
            }
            catch
            {
                return (T)Convert.ChangeType(null, typeof(T));
            }


        }

        public void SetValue(string s, dynamic value)
        {
            try
            {
                if (_structs.ContainsKey(s))
                {
                    Dictionary<uint, uint[]> structDetails = _structs[s];
                    uint bufferSize = structDetails.ElementAt(0).Key;
                    uint[] offsets = structDetails.ElementAt(0).Value;

                    ulong memoryLocation = Memory.ptrChain(_basePTR, offsets);
                    processMethods.WriteMemory(memoryLocation, value);
                }
            }
            catch { }
        }
    }
}
