#!/usr/bin/env bash
set -ex

npm install

cd Bootstrapper
./build.sh
cd ..

cd DemoApplication
./build.sh
cd ..

cd RuntimePatcher
./build.sh
cd ..
