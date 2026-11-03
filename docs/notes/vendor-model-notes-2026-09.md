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
- **2026-10-07, Codex CLI with a ChatGPT account:** accepts `gpt-6-astra`, `gpt-5.6-sol` and `gpt-5.6-terra`. Rejects `gpt-6.1-sol`, `gpt-6-sol` and `gpt-6-luna` ("not supported when using Codex with a ChatGPT account"). The Codex app offers all of them. Until this changes, the Codex standard tier means `gpt-5.6-sol`, and the top tier means `gpt-6-astra`. (Found when WO-2026-10-07-002's Stage 7 round 2 dispatch was rejected on `gpt-6.1-sol`; Codex CLI 0.154.0.)
- **2026-10-22, launching `codex exec` from a script:** close stdin (`codex exec … </dev/null`). Without it the CLI prints "Reading additional input from stdin..." and waits forever (one S-6 Stage 7 run sat 52 minutes with no output). **Ten minutes with no output means hung: kill it and relaunch.**
- **2026-11-03, `codex exec -o <file>` overwrites the file with the agent's last message.** If Codex writes a full verdict to that path during the run and then ends with a short summary, the summary replaces it (seen twice on 3 Nov: WO-2026-11-02-003 round 1 and WO-2026-11-02-004 round 1; the full text had to be recovered from the run log). The dispatch therefore asks for the complete verdict in the final message, and Claude Code checks the saved file's length before relying on it.
- **2026-11-03, Stage 7 verdicts are valid only for the commit they name.** The rule (the `UNDER REVIEW` header line, per-round dispatch and verdict file names, and the SHA on the verdict's first line) is in `AI_DEVELOPMENT_WORKFLOW.md` §4. Background: a round-2 run on 3 Nov was fed the round-1 dispatch file by mistake and reviewed the superseded change.

## Open question, not yet resolved

Whether Astra's context-notes architecture would meaningfully change how many rounds a Stage-7 review needs before converging, compared to the GPT-5.6 Sol family in current use — no data, and not being tested at this time per the standard routing above.
