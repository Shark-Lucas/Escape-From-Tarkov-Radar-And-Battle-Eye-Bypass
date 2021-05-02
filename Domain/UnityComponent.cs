using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ExtProject.Domain
{
    // Unused
    class UnityComponent
    {
        public UnityComponent(ulong componentAddress, ulong componentClassAddress)
        {
            ComponentAddress = componentAddress;
            ComponentClassAddress = componentClassAddress;
        }

        public readonly ulong ComponentAddress;
        public readonly ulong ComponentClassAddress;
        public string Name;
        public string Namespace;

        public override string ToString()
        {
            return $"{(string.IsNullOrWhiteSpace(Namespace) ? "-" : Namespace)}.{Name}";
        }

        public static implicit operator ulong(UnityComponent component) => component.ComponentClassAddress;
    }
}
