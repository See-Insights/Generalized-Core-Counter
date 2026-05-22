#!/usr/bin/env bash

# Release helper: bump version + notes, update Doxyfile + README + CHANGELOG,
# generate Doxygen docs, commit, and push.
#
# Usage:
#   ./release.sh <version> <release-notes...>
#   ./release.sh --tag <version> <release-notes...>
#   ./release.sh --yes --tag <version> <release-notes...>
#   ./release.sh --dry-run <version> <release-notes...>
#
# Notes:
# - Uses bump_version.sh (macOS/BSD sed).
# - Allows dirty working tree by default (pass --require-clean to block).

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

DO_TAG=0
ASSUME_YES=0
DRY_RUN=0
ALLOW_DIRTY=1

usage() {
  cat <<'EOF'
Usage:
  ./release.sh [--yes] [--tag] [--dry-run] [--allow-dirty] [--require-clean] <version> <release-notes...>

Options:
  --tag         Create annotated git tag v<version> and push it
  --yes         Skip confirmation prompt
  --dry-run     Print commands but do not change anything
  --allow-dirty Allow running even if git status is not clean
  --require-clean Fail if git status is not clean (overrides default)
EOF
}

run() {
  if [ "$DRY_RUN" -eq 1 ]; then
    echo "+ $*"
  else
    "$@"
  fi
}

# Parse flags
while [ "$#" -gt 0 ]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --tag)
      DO_TAG=1
      shift
      ;;
    --yes)
      ASSUME_YES=1
      shift
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    --allow-dirty)
      ALLOW_DIRTY=1
      shift
      ;;
    --require-clean)
      ALLOW_DIRTY=0
      shift
      ;;
    --)
      shift
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
    *)
      break
      ;;
  esac
done

if [ "$#" -lt 2 ]; then
  usage
  exit 2
fi

VERSION="$1"
shift
RELEASE_NOTES="$*"

if [ ! -x "./bump_version.sh" ]; then
  echo "Error: bump_version.sh not found or not executable" >&2
  exit 1
fi

if ! command -v git >/dev/null 2>&1; then
  echo "Error: git not found" >&2
  exit 1
fi

if ! command -v doxygen >/dev/null 2>&1; then
  echo "Error: doxygen not found (install Doxygen or update PATH)" >&2
  exit 1
fi

if [ -z "$(git rev-parse --show-toplevel 2>/dev/null || true)" ]; then
  echo "Error: not a git repository" >&2
  exit 1
fi

if [ "$ALLOW_DIRTY" -ne 1 ]; then
  if [ -n "$(git status --porcelain)" ]; then
    echo "Error: working tree has uncommitted changes." >&2
    echo "Commit/stash them first, or pass --allow-dirty." >&2
    exit 1
  fi
fi

# Ensure we have an upstream branch (for push)
CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
UPSTREAM="$(git rev-parse --abbrev-ref --symbolic-full-name @{u} 2>/dev/null || true)"
if [ -z "$UPSTREAM" ]; then
  echo "Error: current branch '$CURRENT_BRANCH' has no upstream remote configured." >&2
  echo "Run: git push -u origin $CURRENT_BRANCH" >&2
  exit 1
fi

COMMIT_MSG="Release ${VERSION}: ${RELEASE_NOTES}"
TAG_NAME="v${VERSION}"

if [ "$ASSUME_YES" -ne 1 ]; then
  echo "About to:"
  echo "  - bump version to ${VERSION}"
  echo "  - update src/Version.cpp, Doxyfile, README.md, CHANGELOG.md"
  echo "  - generate Doxygen docs (docs/html)"
  echo "  - git commit -m '${COMMIT_MSG}'"
  echo "  - git push (${UPSTREAM})"
  if [ "$DO_TAG" -eq 1 ]; then
    echo "  - git tag -a ${TAG_NAME}"
    echo "  - git push --tags"
  fi
  echo
  if ! read -r -p "Continue? [y/N] " ANSWER; then
    ANSWER=""
  fi
  case "${ANSWER}" in
    y|Y|yes|YES) ;;
    *) echo "Aborted."; exit 0;;
  esac
fi

# Step 1: bump version/notes + doc version + README + changelog entry
run ./bump_version.sh "$VERSION" "$RELEASE_NOTES"

# Step 2: generate Doxygen docs
run doxygen Doxyfile

# Step 3: stage known files
run git add src/Version.cpp Doxyfile README.md CHANGELOG.md docs/html

# Step 4: commit
if [ -n "$(git diff --cached --name-only)" ]; then
  run git commit -m "$COMMIT_MSG"
else
  echo "Nothing staged to commit; skipping commit." >&2
fi

# Step 5: tag (optional)
# Step 5: tag (optional)
if [ "$DO_TAG" -eq 1 ]; then
  if git rev-parse -q --verify "refs/tags/${TAG_NAME}" >/dev/null; then
    echo "Error: tag ${TAG_NAME} already exists." >&2
    exit 1
  fi
  run git tag -a "$TAG_NAME" -m "$COMMIT_MSG"
fi

# Step 6: push
run git push
if [ "$DO_TAG" -eq 1 ]; then
  run git push --tags
fi

echo "Done."
