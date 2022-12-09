start DemoApplication\dist\DemoApplication.exe
npm start -- inject ^
DemoApplication.exe ^
Bootstrapper\build\Release\Bootstrapper.dll ^
RuntimePatcher\dist\RuntimePatcher.runtimeconfig.json ^
RuntimePatcher\dist\RuntimePatcher.dll ^
"RuntimePatcher.Main, RuntimePatcher" "InitializePatches"
