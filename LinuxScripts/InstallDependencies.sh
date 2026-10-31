#!/bin/sh
set -eu

sudo apt update
sudo apt-get install -y libgl1-mesa-dev
sudo apt-get install -y mesa-utils
sudo apt install -y libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev

sh "$(dirname "$0")/InstallVCPKG.sh"
