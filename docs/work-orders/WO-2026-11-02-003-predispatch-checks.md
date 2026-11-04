# WO-2026-11-02-003: pre-dispatch checks (Claude Code, 2 Nov 2026)

**Actions enabled (Amendment 1, item 3).**
- Repository `See-Insights/Generalized-Core-Counter` (public): `GET /repos/.../actions/permissions` returns `enabled: true`, `allowed_actions: "all"`, `sha_pinning_required: false`.
- Actions has run on this repository: `pages build and deployment` runs succeeded on `main` on 1 Nov 2026 (`gh run list`).
- **The organization-level policy could not be read:** `GET /orgs/See-Insights/actions/permissions` needs `admin:org` (HTTP 403). A repository showing `enabled: true` is not blocked by an organization policy, so this is satisfied for this repository; Chip can confirm the org setting if he wants it in writing. No token scope was changed.

**Current releases and the SHAs to pin (resolved 2 Nov 2026 with `gh api`; tags are commit tags, so the tag SHA is the commit SHA).**

| Action | Latest release | Published | Commit SHA to pin |
|---|---|---|---|
| `actions/checkout` | v7.0.1 | 2026-07-20 | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| `actions/upload-artifact` | v7.0.2 | 2026-10-07 | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |
| `particle-iot/compile-action` | v1.0.7 | 2026-05-14 | `3b6e15e99da59d074120f854a71659d4c81f092c` |

Re-resolve at dispatch time if the day changes the latest release; keep the tag in a comment on each `uses:` line. Amendment 1 says "v4 or later": v7 satisfies it. Phase 2 (flash and firmware-upload actions) is out of scope and needs Chip's decision on tokens.
