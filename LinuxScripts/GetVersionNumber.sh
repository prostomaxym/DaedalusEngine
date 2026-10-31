#!/bin/bash
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

version_number=""
while IFS= read -r line || [ -n "$line" ]; do
	line=${line%$'\r'}
	[ -z "$line" ] && continue
	version_number+="${line##* }."
done < version.txt

echo "${version_number%.}"
