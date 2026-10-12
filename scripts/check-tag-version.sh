#!/usr/bin/env bash
# Checks a release tag against the project version in CMakeLists.txt.
#   check-tag-version.sh <tag> [<CMakeLists.txt>]    prints the version if <tag> is v<version>
#   check-tag-version.sh --print [<CMakeLists.txt>]  prints the version
set -euo pipefail
tag=${1:?usage: check-tag-version.sh <tag>|--print [<CMakeLists.txt>]}
cmakelists=${2:-CMakeLists.txt}
version=$(sed -nE 's/^project\([^)]*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$cmakelists" | head -n 1)
if [ -z "$version" ]; then
    echo "No project version found in $cmakelists" >&2
    exit 1
fi
if [ "$tag" != "--print" ] && [ "$tag" != "v$version" ]; then
    echo "Tag $tag does not match the version $version in CMakeLists.txt" >&2
    exit 1
fi
echo "$version"
