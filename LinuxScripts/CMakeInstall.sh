#!/bin/bash
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

cmake --install build --prefix runtime/install --config Release
