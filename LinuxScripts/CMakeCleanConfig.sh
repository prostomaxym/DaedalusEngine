#!/bin/bash
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

rm -rf build build-debug build-release-devtools

if [ -d build ] || [ -d build-debug ] || [ -d build-release-devtools ]; then
	echo "Can't delete build folders. Close all projects and retry"
	exit 1
fi

bash "$repo_root/LinuxScripts/CMakeConfigAndPackages.sh"
