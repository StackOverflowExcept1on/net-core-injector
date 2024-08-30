#!/usr/bin/env bash
set -ex
dotnet publish -c Release -r linux-x64 -p:PublishDir="$(pwd)/dist" --self-contained
