#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
./ThirdParty/vcpkg/vcpkg install --triplet=x64-linux --overlay-triplets="$repo_root/ThirdParty/vcpkg_triplets"
