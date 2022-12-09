### net-core-injector


[![Build Status](https://github.com/StackOverflowExcept1on/net-core-injector/actions/workflows/ci.yml/badge.svg)](https://github.com/StackOverflowExcept1on/net-core-injector/actions/workflows/ci.yml)

![banner](assets/banner.gif)

CLI tool that can replace C# methods in .NET Core applications

### Requirements

- Visual Studio 2022 with installed C++ & C# build tools: https://visualstudio.microsoft.com/en/vs/
- Node.js: https://nodejs.org/en/download/
- frida: https://frida.re

### Installation on Windows

Open command line and run this script

It will build

- Node.js package [net-core-injector](package.json) - DLL-injector written in TypeScript
- [Bootstrapper](Bootstrapper) - helper native library written in C++ to interact with .NET Core runtime
- [DemoApplication](DemoApplication) - test application to demonstrate how it works, 
- [RuntimePatcher](RuntimePatcher) - code that attaches to [DemoApplication](DemoApplication)

```
_build.bat
```

### Running

This script should produce output like the GIF above

```
_run.bat
```

### Internal documentation

It's mostly based on Microsoft documentation:
[Write a custom .NET host to control the .NET runtime from your native code](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)

TL;DR: each process that runs on .NET Core uses `hostfxr.dll` or `libhostfxr.so`. This library is loaded in its memory.

To load a custom C# assembly (also known as a DLL), you need to manipulate with `hostfxr` first.
I did it in [`Bootstrapper/src/library.cpp`](Bootstrapper/src/library.cpp).

[`net-core-injector/src/main.ts`](src/main.ts) injects `Bootstrapper.dll` into C# process and loads custom assembly


The following command runs `DemoApplication.exe` on another thread and injects code. 
```
start DemoApplication\dist\DemoApplication.exe

npm start -- inject ^
DemoApplication.exe ^
Bootstrapper\build\Release\Bootstrapper.dll ^
RuntimePatcher\dist\RuntimePatcher.runtimeconfig.json ^
RuntimePatcher\dist\RuntimePatcher.dll ^
"RuntimePatcher.Main, RuntimePatcher" "InitializePatches"
```

Then the execution happens in this order:
1. get into `DemoApplication.exe` process memory via DLL-injection of `Bootstrapper.dll`
2. call native C++ code
   ```cpp
   bootstrapper_load_assembly(
       /*runtime_config_path = */"RuntimePatcher\\dist\\RuntimePatcher.runtimeconfig.json",
       /*assembly_path = */"RuntimePatcher\\dist\\RuntimePatcher.dll",
       /*type_name = */"RuntimePatcher.Main, RuntimePatcher",
       /*method_name = */"InitializePatches"
   )
   ```
3. [`RuntimePatcher/Lib.cs`](RuntimePatcher/RuntimePatcher/Lib.cs) attaches to code of `DemoApplication.exe`

### Application in real world

I injected my DLL into the GitHub Actions security system and received money and a t-shirt from HackerOne

Also see: https://github.com/StackOverflowExcept1on/how-to-hack-github-actions

You can use this to mod games written in C# or to patch any software

### TODO

- Linux support is incomplete due to some issues with the secondary host context in `hostfxr`.
