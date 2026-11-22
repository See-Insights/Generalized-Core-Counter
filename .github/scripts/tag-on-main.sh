#!/usr/bin/env bash
# Fails unless <sha> is an ancestor of (or equal to) <main-ref>: a v* tag on any other branch must not build or upload.
set -euo pipefail
sha="${1:?usage: tag-on-main.sh <sha> <main-ref>}"; ref="${2:?usage: tag-on-main.sh <sha> <main-ref>}"
git rev-parse --verify --quiet "$ref^{commit}" >/dev/null || { echo "::error::$ref not found; cannot check that the tag is on main" >&2; exit 1; }
git merge-base --is-ancestor "$sha" "$ref" || { echo "::error::tagged commit $sha is not on $ref; tags are cut from main only; not building" >&2; exit 1; }
