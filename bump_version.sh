#!/usr/bin/env bash

# Simple helper to bump firmware version, release notes, Doxygen project number,
# README version header, and append to the changelog.
#
# Usage:
#   ./bump_version.sh 3.02 "Fix PIR debounce timing"
#
# Notes:
# - This script is tailored for macOS (uses BSD sed with -i '').
# - Escapes values for safe use in sed replacement.

set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <version> <release-notes>"
  exit 1
fi

VERSION="$1"
shift
NOTES="$*"

sed_escape_replacement() {
  # Escapes characters that are special in the sed replacement part.
  # - '&' expands to the matched text
  # - '\\' is the escape character
  # - '/' is the delimiter we use
  # - '"' must be escaped because we embed the replacement in a double-quoted sed script
  printf '%s' "$1" | sed -e 's/[\\&/]/\\&/g' -e 's/"/\\"/g'
}

VERSION_ESCAPED="$(sed_escape_replacement "$VERSION")"
NOTES_ESCAPED="$(sed_escape_replacement "$NOTES")"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

VERSION_FILE="src/Version.cpp"
DOXYFILE="Doxyfile"
CHANGELOG="CHANGELOG.md"
README="README.md"

if [ ! -f "$VERSION_FILE" ]; then
  echo "Error: $VERSION_FILE not found"
  exit 1
fi

if [ ! -f "$DOXYFILE" ]; then
  echo "Error: $DOXYFILE not found"
  exit 1
fi

if [ ! -f "$README" ]; then
  echo "Error: $README not found"
  exit 1
fi

# Update firmware version in Version.cpp
sed -i '' "s/^const char\\* FIRMWARE_VERSION.*/const char* FIRMWARE_VERSION = \"${VERSION_ESCAPED}\";/" "$VERSION_FILE"

# Update firmware release notes in Version.cpp
sed -i '' "s/^const char\\* FIRMWARE_RELEASE_NOTES.*/const char* FIRMWARE_RELEASE_NOTES = \"${NOTES_ESCAPED}\";/" "$VERSION_FILE"

# Update Doxygen project number
sed -i '' "s/^PROJECT_NUMBER.*/PROJECT_NUMBER         = \"${VERSION_ESCAPED}\"/" "$DOXYFILE"

# Update README version line
sed -i '' "s/^\*\*Version:\*\*.*/\*\*Version:\*\* ${VERSION_ESCAPED} | \*\*Latest:\*\* ${NOTES_ESCAPED}/" "$README"

# Append to CHANGELOG.md
DATE="$(date +%Y-%m-%d)"
{
  echo "## ${VERSION} – ${DATE}"
  echo "- ${NOTES}"
  echo
} >> "$CHANGELOG"

echo "Updated to version ${VERSION}"