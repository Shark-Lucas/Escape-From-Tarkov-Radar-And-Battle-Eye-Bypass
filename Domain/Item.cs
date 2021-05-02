using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;

namespace ExtProject.Domain
{
    class Item
    {
        public ulong Address = 0;
        public Vector3 location;
        public String name;

        public Item(ulong addr)
        {
            Address = addr;
        }

    }
}