#!/usr/bin/env bash
# Apply our patches to the SDK checkout, in order.
#
# Usage: bash tools/apply-sdk-patches.sh <sdk checkout> <patch folder>
#
# The patches are first applied one after another to a scratch index built from the SDK's
# commit, so the working tree is only touched when the whole series fits:
# - a patch that applies goes into the series;
# - a patch whose changes the SDK already has (it reverses cleanly) is reported and left out,
#   which is what happens once upstream has taken it;
# - any other patch stops the script with an error: it has to be updated or removed for this
#   SDK version.
# Then the working tree has to match either the SDK's commit (the series is applied) or the
# commit with the series (nothing to do). Anything else is a local change, and the script stops
# rather than patch over it.
set -euo pipefail

sdk="$(cd "$1" && pwd)"
patch_dir="$(cd "$2" && pwd)"

scratch_index="$(mktemp)"
trap 'rm -f "$scratch_index"' EXIT
# core.safecrlf off: comparing a CRLF checkout with the index is fine, the warnings are noise.
scratch() { GIT_INDEX_FILE="$scratch_index" git -C "$sdk" -c core.safecrlf=false "$@"; }
scratch read-tree HEAD

series=()
for patch in "$patch_dir"/*.patch; do
  [ -e "$patch" ] || continue
  name="$(basename "$patch")"
  if scratch apply --cached --check "$patch" 2>/dev/null; then
    scratch apply --cached "$patch"
    series+=("$patch")
  elif scratch apply --cached --reverse --check "$patch" 2>/dev/null; then
    echo "already in the SDK: $name (remove the patch)"
  else
    echo "error: $name does not apply to this SDK version:" >&2
    scratch apply --cached --check "$patch" >&2 || true
    echo "Update the patch, or remove the part the SDK already has." >&2
    exit 1
  fi
done

if scratch diff --quiet --ignore-submodules; then
  echo "patches already applied (${#series[@]})"
  exit 0
fi
if ! git -C "$sdk" -c core.safecrlf=false diff --quiet --ignore-submodules HEAD; then
  echo "error: $sdk has local changes that are not our patches:" >&2
  git -C "$sdk" -c core.safecrlf=false diff --stat --ignore-submodules HEAD >&2
  echo "Commit, stash or undo them, then run this again." >&2
  exit 1
fi
for patch in "${series[@]}"; do
  git -C "$sdk" apply "$patch"
  echo "applied $(basename "$patch")"
done
