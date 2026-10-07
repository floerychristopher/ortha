#!/usr/bin/env bash
# Stop hook: builds ortha and runs the unit tests when Claude finishes a turn.
# Green: silent. Red: exit code 2 hands the last lines back to Claude, once per turn
# (stop_hook_active is set when Claude is already continuing because of this hook).
input=$(cat)
if [ "$(jq -r '.stop_hook_active // false' <<<"$input")" = "true" ]; then
    exit 0
fi
cd "${CLAUDE_PROJECT_DIR:-$(dirname "$0")/../..}" || exit 0
[ -d build ] || exit 0 # not configured yet

log=$(mktemp)
trap 'rm -f "$log"' EXIT
if cmake --build build >"$log" 2>&1 && ctest --test-dir build --output-on-failure >>"$log" 2>&1; then
    exit 0
fi
{
    echo "ortha build or unit tests failed (Stop hook). Last lines:"
    tail -n 40 "$log"
} >&2
exit 2
