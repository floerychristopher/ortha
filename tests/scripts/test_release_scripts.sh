#!/usr/bin/env bash
# Tests for the release scripts and the Scoop manifest. Usage: test_release_scripts.sh <repo root>
set -euo pipefail
root=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail() { echo "FAIL: $*" >&2; exit 1; }

# --- check-tag-version.sh -------------------------------------------------------------
cat > "$tmp/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.25)
project(ortha VERSION 0.2.0 LANGUAGES C CXX)
CMAKE
out=$("$root/scripts/check-tag-version.sh" v0.2.0 "$tmp/CMakeLists.txt") || fail "v0.2.0 should match"
[ "$out" = "0.2.0" ] || fail "expected version 0.2.0 on stdout, got '$out'"
for tag in v0.2.1 0.2.0 v0.2 v0.2.0-rc1; do
    if err=$("$root/scripts/check-tag-version.sh" "$tag" "$tmp/CMakeLists.txt" 2>&1); then
        fail "$tag should not match"
    fi
    [[ "$err" == *"Tag $tag does not match the version 0.2.0 in CMakeLists.txt"* ]] ||
        fail "unexpected message for $tag: $err"
done
# the real CMakeLists.txt has a version the script can read
"$root/scripts/check-tag-version.sh" "v$("$root/scripts/check-tag-version.sh" --print "$root/CMakeLists.txt")" \
    "$root/CMakeLists.txt" >/dev/null || fail "the repo's own version"

# --- bucket/ortha.json ----------------------------------------------------------------
python3 -I - "$root/bucket/ortha.json" <<'PY' || fail "bucket/ortha.json is not a valid manifest"
import json, sys
m = json.load(open(sys.argv[1], encoding="utf-8"))
for key in ("version", "description", "homepage", "license", "url", "hash", "bin", "checkver", "autoupdate"):
    assert key in m, key
assert m["bin"] == "ortha.exe"
assert m["url"].endswith("/v%s/ortha-%s-windows-x64.zip" % (m["version"], m["version"]))
assert m["autoupdate"]["url"].endswith("/v$version/ortha-$version-windows-x64.zip")
PY

# --- update-scoop-manifest.py ---------------------------------------------------------
cp "$root/bucket/ortha.json" "$tmp/ortha.json"
hash=$(printf 'a%.0s' {1..64})
"$root/scripts/update-scoop-manifest.py" "$tmp/ortha.json" 0.3.0 "$hash" || fail "update should work"
python3 -I - "$root/bucket/ortha.json" "$tmp/ortha.json" "$hash" <<'PY' || fail "manifest not updated as expected"
import json, sys
before = json.load(open(sys.argv[1], encoding="utf-8"))
after = json.load(open(sys.argv[2], encoding="utf-8"))
assert after["version"] == "0.3.0"
assert after["url"].endswith("/v0.3.0/ortha-0.3.0-windows-x64.zip"), after["url"]
assert after["hash"] == sys.argv[3]
assert list(after) == list(before), "field order changed"
for key in before:
    if key not in ("version", "url", "hash"):
        assert after[key] == before[key], key
PY
cp "$tmp/ortha.json" "$tmp/once.json"
"$root/scripts/update-scoop-manifest.py" "$tmp/ortha.json" 0.3.0 "$hash"
cmp -s "$tmp/ortha.json" "$tmp/once.json" || fail "a second run changed the file"
cp "$tmp/ortha.json" "$tmp/kept.json"
if "$root/scripts/update-scoop-manifest.py" "$tmp/ortha.json" 0.4.0 "nothex" 2>/dev/null; then
    fail "an invalid hash should be rejected"
fi
cmp -s "$tmp/ortha.json" "$tmp/kept.json" || fail "a rejected update changed the file"
for bad_version in "" "v0.4.0" "0.4" "latest"; do
    if "$root/scripts/update-scoop-manifest.py" "$tmp/ortha.json" "$bad_version" "$hash" 2>/dev/null; then
        fail "version '$bad_version' should be rejected"
    fi
done
cmp -s "$tmp/ortha.json" "$tmp/kept.json" || fail "a rejected version changed the file"

# --- .github/workflows/release.yml ----------------------------------------------------
python3 -I - "$root/.github/workflows/release.yml" <<'PY' || fail "release.yml is not as expected"
import sys
try:
    import yaml
except ImportError:  # optional: without PyYAML the structure check is skipped
    print("note: PyYAML missing, release.yml structure not checked")
    sys.exit(0)
wf = yaml.safe_load(open(sys.argv[1], encoding="utf-8"))
trigger = wf.get("on", wf.get(True))  # YAML 1.1 reads a bare "on" as true
assert trigger == {"push": {"tags": ["v*"]}}, trigger
jobs = wf["jobs"]
assert set(jobs) == {"check-version", "build", "release", "scoop"}, set(jobs)
assert jobs["build"]["needs"] == "check-version"
assert set(jobs["release"]["needs"]) == {"check-version", "build"}
assert set(jobs["scoop"]["needs"]) == {"check-version", "release"}
assert wf["permissions"] == {"contents": "read"}
for name in ("release", "scoop"):
    assert jobs[name]["permissions"] == {"contents": "write"}, name
text = open(sys.argv[1], encoding="utf-8").read()
# The tag check must be its own command: inside echo "...$(...)" its failure would not stop
# the step (bash -e ignores failing command substitutions in arguments).
assert 'echo "version=$(scripts/check-tag-version.sh' not in text
assert 'version=$(scripts/check-tag-version.sh "$GITHUB_REF_NAME")' in text
for needle in ("scripts/check-tag-version.sh", "scripts/update-scoop-manifest.py",
               "SHA256SUMS.txt", "windows-x64.zip", "linux-x64.tar.gz",
               "chore(scoop): ortha", "--generate-notes", "ref: main"):
    assert needle in text, needle
PY

echo "release scripts: ok"
