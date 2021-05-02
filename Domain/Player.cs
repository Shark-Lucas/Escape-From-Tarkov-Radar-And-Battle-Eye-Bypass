using ExtProject.MemoryManagement;
using System;
using System.Collections.Generic;
using UnityEngine;

namespace ExtProject.Domain
{
    class Player
    {
        public EFTStruct structs = null;
        public ulong Address = 0;
        public String name;
        public bool isPlayer;
        public Vector3 location;
        public String id;
        public float distance = 0f;
        public Player(ulong addr)
        {
            Address = addr;

            structs = new EFTStruct(Address, new Dictionary<string, Dictionary<uint, uint[]>>() {
            { "HEAD_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0x30, 0x10, 0x10 } } } },
            { "CHEST_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0x48, 0x10, 0x10 } } } },
            { "STOMACH_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0x60, 0x10, 0x10 } } } },
            { "LEFTARM_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0x78, 0x10, 0x10 } } } },
            { "RIGHTARM_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0x90, 0x10, 0x10 } } } },
            { "LEFTLEG_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0xA8, 0x10, 0x10 } } } },
            { "RIGHTLEG_CUR", new Dictionary<uint, uint[]>(){ { 4, new uint[] { 0x400, 0x18, 0x18, 0xC0, 0x10, 0x10 } } } },
            });
        }


        public float GetCurHealth()
        {
            float hp = 0f;
            // FillAll();
            hp += structs.GetValue<float>("HEAD_CUR");
            hp += structs.GetValue<float>("CHEST_CUR");
            hp += structs.GetValue<float>("STOMACH_CUR");
            hp += structs.GetValue<float>("LEFTARM_CUR");
            hp += structs.GetValue<float>("RIGHTARM_CUR");
            hp += structs.GetValue<float>("LEFTLEG_CUR");
            hp += structs.GetValue<float>("RIGHTLEG_CUR");
            return hp;
        }
        public void FillAll()
        {
            structs.SetValue("HEAD_CUR", structs.GetValue<float>("HEAD_MAX"));
            structs.SetValue("CHEST_CUR", structs.GetValue<float>("CHEST_MAX"));
            structs.SetValue("STOMACH_CUR", structs.GetValue<float>("STOMACH_MAX"));
            structs.SetValue("LEFTARM_CUR", structs.GetValue<float>("LEFTARM_MAX"));
            structs.SetValue("RIGHTARM_CUR", structs.GetValue<float>("RIGHTARM_MAX"));
            structs.SetValue("LEFTLEG_CUR", structs.GetValue<float>("LEFTLEG_MAX"));
            structs.SetValue("RIGHTLEG_CUR", structs.GetValue<float>("RIGHTLEG_MAX"));
        }

        public float GetDirection()
        {
            var movementContext = Memory.Read<UInt64>(Address + 0x38);
            float deg = Memory.Read<float>(movementContext + 0x1E0);
            if (deg < 0)
            {
                return 360f + deg;
            }
            return deg;
        }
    }
}
