#!/usr/bin/env zsh
# .github/scripts/release-title.sh: the Particle title comes only from line 1 "Title: <summary>" of docs/releases/<tag>-notes.md.
set -u
root=${0:A:h:h}; s=$root/.github/scripts/release-title.sh
d=$(mktemp -d "${TMPDIR:-/tmp}/rt.XXXXXX"); trap 'rm -rf "$d"' EXIT
mkdir -p $d/docs/releases; cd $d
fail=0; chk() { [[ "$2" == "$3" ]] && echo "ok   $1" || { echo "FAIL $1: got '$2' want '$3'"; fail=1; }; }
printf 'Title: AM1805 on Device OS driver, watchdog fix  \nbody\n' > docs/releases/v1.1-notes.md
chk good "$($s v1.1 2>/dev/null)" "Release v1.1: AM1805 on Device OS driver, watchdog fix"
printf 'Title: CRLF case\r\nbody\r\n' > docs/releases/v1.2-notes.md
chk crlf "$($s v1.2 2>/dev/null)" "Release v1.2: CRLF case"
printf 'DRAFT\nTitle: not on line 1\n' > docs/releases/v1.3-notes.md
$s v1.3 >/dev/null 2>&1; chk not-line-1 "$?" 1
printf 'Title:   \nbody\n' > docs/releases/v1.4-notes.md
$s v1.4 >/dev/null 2>&1; chk empty-summary "$?" 1
$s v1.5 >/dev/null 2>&1; chk missing-file "$?" 1
$s >/dev/null 2>&1; chk no-arg "$?" 1
# the real notes for the current release must pass
cd $root; for f in docs/releases/v50.1-notes.md; do t=${${f:t}%-notes.md}; chk "repo $t" "$($s $t 2>/dev/null | cut -c1-14)" "Release ${t}:"; done
# build.yml wiring: firmware job computes the title for tags; upload takes it from there
w=$root/.github/workflows/build.yml
chk wiring-script "$(grep -c 'release-title.sh "$GITHUB_REF_NAME"' $w)" 1
chk wiring-output "$(grep -c 'title: ${{ steps.title.outputs.title }}' $w)" 1
chk wiring-upload "$(grep -c 'TITLE: "${{ needs.firmware.outputs.title }}"' $w)" 1
chk no-old-title "$(grep -c 'Development Release' $w)" 0
exit $fail
