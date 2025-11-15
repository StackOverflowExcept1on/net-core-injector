#!/usr/bin/env bash
set -e

while getopts "a" OPTION 2> /dev/null; do
  case ${OPTION} in
    a)
      DO_ATTACH="yes"
      ;;
    \?)
      break
      ;;
  esac
done

if [ "$DO_ATTACH" == "yes" ]; then
  set -m
  sudo sysctl kernel.yama.ptrace_scope=0

  ./DemoApplication/dist/DemoApplication &
  npm start -- inject \
  DemoApplication \
  Bootstrapper/build/bin/libBootstrapper.so \
  RuntimePatcher/dist/RuntimePatcher.runtimeconfig.json \
  RuntimePatcher/dist/RuntimePatcher.dll \
  "RuntimePatcher.Main, RuntimePatcher" \
  "InitializePatches"
  fg %1
else
  LD_PRELOAD=./Bootstrapper/build/bin/libBootstrapper.so \
  RUNTIME_CONFIG_PATH="$(pwd)/RuntimePatcher/dist/RuntimePatcher.runtimeconfig.json" \
  ASSEMBLY_PATH="$(pwd)/RuntimePatcher/dist/RuntimePatcher.dll" \
  TYPE_NAME="RuntimePatcher.Main, RuntimePatcher" \
  METHOD_NAME="InitializePatches" \
  ./DemoApplication/dist/DemoApplication
fi
