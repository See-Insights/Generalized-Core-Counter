# Particle API users register (no tokens, ever)

Policy: WO-2026-11-02-003 Amendment 2 and WO-2026-11-03-003. Product API users only (never a human user token), one per function, one scope each, scoped to product 42131. Tokens live only in GitHub Environment secrets (`particle-42131`: required reviewer Chip, `v*` tags only), never in the repo, logs or command-line arguments. CI never releases firmware or sets a default: Chip does that in the Console. API-user tokens do not expire, so rotate quarterly and at once on any suspected exposure; delete an API user when its job is retired.

| Name | Product | Scope | Created | Last rotated | Where the token lives | Used by |
|---|---|---|---|---|---|---|
| `github-firmware-upload` (name as relayed by Chip on 3 Nov; confirm against the Console) | 42131 | `firmware:create` only | 3 Nov 2026 (created by Chip) | not yet (next rotation due by 3 Feb 2027) | GitHub Environment `particle-42131`, secret `PARTICLE_ACCESS_TOKEN` | WO-2026-11-03-003 Part B: `particle-iot/firmware-upload-action` on `v*` tags; uploads only |

Scope failures: if an upload is refused for scope, record the exact scope the error names here and report it to Chip. No broader scope is granted to make it work; the change is decided case by case, by Chip.
