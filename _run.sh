#!/usr/bin/env bash
set -ex

sudo sysctl kernel.yama.ptrace_scope=0
./DemoApplication/dist/DemoApplication &
npm start -- inject \
DemoApplication \
Bootstrapper/build/libBootstrapper.so \
RuntimePatcher/dist/RuntimePatcher.runtimeconfig.json \
RuntimePatcher/dist/RuntimePatcher.dll \
"RuntimePatcher.Main, RuntimePatcher" "InitializePatches"
