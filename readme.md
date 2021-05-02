# Escape From Tarkov Radar and BattleEye Bypass

Disclaimer: This repository is not for exploiting the game and purely for educational purposes.

This project was just for fun and a learning experience regarding memory management, kernel mode and user mode concepts and drivers. This code is very, very rough, and lots of improvements could be made: Caching certain parts of memory to avoid making wasted calls, using some sort of pattern matching or library to dynamically pull certain addresses instead of manually updating when the game updates, using Unity to project a top-down image of the map onto the radar, etc. Features aside, thee code itself is very sloppy and was more of a proof-of-concept. 

## Bypass

Using this [EFI driver](https://github.com/TheCruZ/EFI_Driver_Access/tree/a440c1f1eb6fb30915f3ee7b5bdc019b2a40deed) and modyifing the provided EFI client, we're able to read and write to and from memory without BattleEye noticing. Using this, we walk the [PEB](https://en.wikipedia.org/wiki/Process_Environment_Block#:~:text=In%20computing%20the%20Process%20Environment,other%20than%20the%20operating%20system.) to get Tarkov's base memory address, as BattleEye removes the handler to the process. With this, we're able to target our pseudo read and write process memory calls to the correct process. This functionality is then abstracted away to a DLL which is consumed in the main C# program, like so:

```
[DllImport("TarkyDriver.dll", CharSet = CharSet.None, ExactSpelling = false, CallingConvention = CallingConvention.Cdecl)]
            
internal static extern ulong read_memory(int hProcess, ulong lpBaseAddress, byte[] buffer, int size);
```

Using a combination of debuggers and tools, we're able to find the memory addresses of some pretty interesting things, such as a guns recoil stats, or a given players location. As we're able to read and write from process memory, we're able to either overwrite the values at these memory addresses, or read from them and interpret the information there accordingly.
