#!/usr/bin/env bash

# Simple helper to bump firmware version, release notes, Doxygen project number,
# and append to the changelog.
#
# Usage:
#   ./bump_version.sh 3.02 "Fix PIR debounce timing"
#
# Notes:
# - This script is tailored for macOS (uses BSD sed with -i '').
# - Avoid double quotes in the release notes, or edit Version.cpp manually.

set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <version> <release-notes>"
  exit 1
fi

VERSION="$1"
shift
NOTES="$*"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

VERSION_FILE="src/Version.cpp"
DOXYFILE="Doxyfile"
CHANGELOG="CHANGELOG.md"

if [ ! -f "$VERSION_FILE" ]; then
  echo "Error: $VERSION_FILE not found"
  exit 1
fi

if [ ! -f "$DOXYFILE" ]; then
  echo "Error: $DOXYFILE not found"
  exit 1
fi

# Update firmware version in Version.cpp
sed -i '' "s/^const char\\* FIRMWARE_VERSION.*/const char* FIRMWARE_VERSION = \"${VERSION}\";/" "$VERSION_FILE"

# Update firmware release notes in Version.cpp
sed -i '' "s/^const char\\* FIRMWARE_RELEASE_NOTES.*/const char* FIRMWARE_RELEASE_NOTES = \"${NOTES}\";/" "$VERSION_FILE"

# Update Doxygen project number
sed -i '' "s/^PROJECT_NUMBER.*/PROJECT_NUMBER         = \"${VERSION}\"/" "$DOXYFILE"

# Append to CHANGELOG.md
DATE="$(date +%Y-%m-%d)"
{
  echo "## ${VERSION} – ${DATE}"
  echo "- ${NOTES}"
  echo
} >> "$CHANGELOG"

echo "Updated to version ${VERSION}"