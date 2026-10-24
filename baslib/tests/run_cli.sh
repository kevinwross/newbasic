#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
newbasic="${NEWBASIC:-$repo/build-wsl/src/newbasic-cli/newbasic}"
library="$repo/baslib/baslib.bas"

if [[ ! -x "$newbasic" ]]; then
  echo "missing executable: $newbasic" >&2
  exit 1
fi
if [[ ! -f "$library" ]]; then
  echo "missing library: $library" >&2
  exit 1
fi

normalize() {
  sed -e 's/[[:space:]]\+/ /g' -e 's/^ //' -e 's/ $//' | sed '/^$/d'
}

failures=0
for fixture in "$here"/fixtures/*.bas; do
  name="$(basename "$fixture" .bas)"
  expected="$here/expected/$name.txt"
  if [[ ! -f "$expected" ]]; then
    echo "FAIL $name (missing expected/$name.txt)"
    failures=$((failures + 1))
    continue
  fi

  temp="$(mktemp -d)"
  merged="$temp/merged.bas"
  result="$repo/result.txt"
  cat "$fixture" > "$merged"
  printf '\n' >> "$merged"
  cat "$library" >> "$merged"
  rm -f "$result"

  ( cd "$repo" && "$newbasic" --gpu-shot "$temp/screen.ppm" "$merged" >/dev/null 2>&1 ) || true
  if [[ ! -f "$result" ]]; then
    echo "FAIL $name (result file missing)"
    rm -rf "$temp"
    failures=$((failures + 1))
    continue
  fi

  actual="$temp/actual.txt"
  normalize < "$result" > "$actual"
  rm -f "$result"

  if ! diff -u "$expected" "$actual"; then
    echo "FAIL $name"
    failures=$((failures + 1))
  else
    echo "PASS $name"
  fi
  rm -rf "$temp"
done

if [[ "$failures" -ne 0 ]]; then
  echo "$failures fixture(s) failed"
  exit 1
fi
echo "All baslib CLI fixtures passed."
