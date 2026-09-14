# Agent Model Routing — as of 2026-09-14

Standing reference for which model to invoke on each agent for this project. The frontier moves fast; re-check before assuming this is still current in more than a few weeks.

## Claude Code (this chat, and any Claude Code sessions on the repo)

- **Standard, all work:** Sonnet 5 (`claude-sonnet-5`). This includes long-horizon structural/architecture work (e.g., the ownership map) and adversarial review of prior output — no model-switching between task types on this project going forward.
- **Known gotcha:** Opus 4.8 and Fable do not reliably appear in the CLI's `/model` picker on Max plans, even though they're available in the desktop/web apps (open bug as of this writing). Not relevant to current routing since this project stays on Sonnet 5, but worth knowing if that changes.

## GitHub Copilot (implementation rounds — Stage 7, WO work)

- **Standard:** Sonnet 5 (`claude-sonnet-5`), matching the Claude Code routing above.
- **Current top-tier alternatives, for reference:** Claude Opus 5, GPT-5.6 (Sol/Terra/Luna variants), Grok 4.5.
- **Deprecated as of 2026-09-01:** Claude Opus 4.5/4.6, Claude Sonnet 4.5/4.6, Gemini 3.1 Pro, Raptor Mini. If any saved prompts/configs pin one of these by name, they may silently fail or fall back — worth an audit of anything pinned to an old model ID.
- **Fable 5/5.1 on Copilot:** available but requires enterprise-level model enablement; not on by default even for paid plans. Not in use for this project.

## Codex CLI

- **Standard:** GPT-5.6 Sol.
- **Current top model, for reference:** GPT-6 Astra (shipped 2026-09-03), requires Codex CLI ≥0.153.0. OpenAI has assessed Astra at their **Critical** cybersecurity capability tier under their Preparedness Framework — the first model placed there. Not currently routed to for this project; Sol remains standard until reassessed.

## Open question, not yet resolved

Whether Astra's context-notes architecture would meaningfully change how many rounds a Stage-7 review needs before converging, compared to the GPT-5.6 Sol family in current use — no data, and not being tested at this time per the standard routing above.
