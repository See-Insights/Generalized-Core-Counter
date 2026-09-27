# Vendor model notes — 2026-09

Historical vendor notes as of 2026-09-14. Not a rule; not maintained. Routing lives in AI_DEVELOPMENT_WORKFLOW.md.

Carried over verbatim from the retired `AGENT_MODEL_ROUTING_2026-09.md` (2026-09-25). Its routing lines (the "Standard" model for each agent, and the introductory "standing reference" line) were left out; section headings are shortened to the vendor name.

## Claude Code

- **Known gotcha:** Opus 4.8 and Fable do not reliably appear in the CLI's `/model` picker on Max plans, even though they're available in the desktop/web apps (open bug as of this writing). Not relevant to current routing since this project stays on Sonnet 5, but worth knowing if that changes.

## GitHub Copilot

- **Current top-tier alternatives, for reference:** Claude Opus 5, GPT-5.6 (Sol/Terra/Luna variants), Grok 4.5.
- **Deprecated as of 2026-09-01:** Claude Opus 4.5/4.6, Claude Sonnet 4.5/4.6, Gemini 3.1 Pro, Raptor Mini. If any saved prompts/configs pin one of these by name, they may silently fail or fall back — worth an audit of anything pinned to an old model ID.
- **Fable 5/5.1 on Copilot:** available but requires enterprise-level model enablement; not on by default even for paid plans. Not in use for this project.

## Codex CLI

- **Current top model, for reference:** GPT-6 Astra (shipped 2026-09-03), requires Codex CLI ≥0.153.0. OpenAI has assessed Astra at their **Critical** cybersecurity capability tier under their Preparedness Framework — the first model placed there. Not currently routed to for this project; Sol remains standard until reassessed.

## Open question, not yet resolved

Whether Astra's context-notes architecture would meaningfully change how many rounds a Stage-7 review needs before converging, compared to the GPT-5.6 Sol family in current use — no data, and not being tested at this time per the standard routing above.
