#!/bin/bash
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

cpack --config build-release-devtools/CPackConfig.cmake -C ReleaseDevTools
