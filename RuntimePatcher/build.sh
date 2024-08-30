#!/usr/bin/env bash
set -ex
dotnet publish -c Release -p:PublishDir="$(pwd)/dist"
