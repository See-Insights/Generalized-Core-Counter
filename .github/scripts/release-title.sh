#!/usr/bin/env bash
# Prints the Particle firmware title for a tag: "Release <tag>: <summary>".
# The summary is the first line of docs/releases/<tag>-notes.md, which must read "Title: <summary>".
# Exits non-zero (with a ::error::) if the file or the line is missing or empty.
set -euo pipefail
tag="${1:?usage: release-title.sh <tag>}"
f="docs/releases/${tag}-notes.md"
[ -f "$f" ] || { echo "::error::$f not found; a tag needs its release notes with a Title: line" >&2; exit 1; }
first=$(head -n 1 "$f" | tr -d '\r')
[[ "$first" =~ ^Title:[[:space:]]*(.*[^[:space:]])[[:space:]]*$ ]] || { echo "::error::line 1 of $f must be 'Title: <summary>' (got '$first')" >&2; exit 1; }
printf 'Release %s: %s\n' "$tag" "${BASH_REMATCH[1]}"
