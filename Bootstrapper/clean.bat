@echo off

set DIRECTORIES=build .vs out .idea cmake-build-debug cmake-build-release

for %%d in (%DIRECTORIES%) do (
  if exist %%d (
    del /s /f /q %%d\*.*
    for /f %%f in ('dir /ad /b %%d\') do rd /s /q %%d\%%f
    rd %%d
  )
)

del CMakeSettings.json
