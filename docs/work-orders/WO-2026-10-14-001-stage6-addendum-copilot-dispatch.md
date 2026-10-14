AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-14, §5) · REASONING: high
AUTHORIZATION SCOPE: as in `WO-2026-10-14-001-stage6-copilot-dispatch.md`, limited to `tests/report_payload_golden_test.{sh,cpp}` and `tests/fixtures/report_payload_golden.txt`. **No `src/` change.** No commits, pushes, stash, reset or checkout.

**SIZE BUDGET:** the whole WO's test lines (added) may now reach **+450**. Stop and report if more is needed.

# Stage 6 addendum: WO-2026-10-14-001 (same round, architect's ruling 2026-10-14)

**Goal:** add the SOC, battery-state, vc and event-name edge vectors in **occupancy** mode too. Today they run in counting mode only. WO 2b-3 merges the two format strings, so each edge case must be pinned in both layouts.

**Do:**
1. **Add occupancy vectors** mirroring the existing counting-only ones:
   - SOC {NaN, ±Inf, −0.01, 0, 87.5, 100, 100.01} × battState {0, 2, 6, 7, 255};
   - vc {unavailable, 3.91, 4.20};
   - event name {"", configured, convention};
   - queue returns false.

   Keep all existing vectors and their ids unchanged.
2. **Append only the new rows to the fixture,** recorded from the **unchanged** `src/`, on this host, with `--dump` or equivalent. The existing rows must stay byte-identical: show `diff` of the old fixture against the new one's existing rows. Keep the host header (Darwin arm64, Apple clang 14).
3. **Re-run** the golden test (all mutants caught) and the full host suite (`N/N (sh via zsh, py via python3)`; it was 77/77).

**Report (your final message):** vectors added and the new total; line counts per file and the WO total against +450; proof that the existing fixture rows are unchanged; the test and suite results; `git diff -- src lib` empty; the model used.
