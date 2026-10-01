AGENT: Codex · MODEL: gpt-6-astra · REASONING: high · via codex exec
Per docs/AI_DEVELOPMENT_WORKFLOW.md. Read-only inventory of software configuration.
AUTHORIZATION SCOPE: read only. No edits, commits, or builds. Exclude lib/ (vendored libraries) and tests/.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-29-001-v27-small-fixes`, HEAD `a6a283c` (v27-SmallFixes), clean working tree apart from this dispatch file.

Goal, in plain language: find every place the firmware is configured, so it can be brought together into a few clear files and the old clutter removed.

1. Known configuration files. Start with these:
Version.h, Version.cpp, FirmwareVersion.h, settings.h, BuildProfile.h, Particle_Functions.h / .cpp, Config.h, Config.cpp, ProjectConfig.h.
For each: its path and a one-line description of what it holds.

2. Find any others. Search src/ only (not lib/, not tests/) for:

#define ENABLE_, #define .*_LEVEL, #define .*_MODE, #define .*_DEBUG, #define .*_TRACE
PRODUCT_VERSION, FIRMWARE_, VERSION
constexpr or static const at file or namespace scope
SerialLogHandler / LogHandler and log-category filters
#if / #ifdef on any of the above

List any file not in step 1 that holds settings, with a one-line description.

3. For every setting found, a table with:

the name, file:line, and current value
where it's read (file:line, or "never")
whether any build sets it differently (bench flags, EXTRA_CFLAGS, #ifndef defaults)
its kind: build switch (logging, diagnostics, trace, bench features), tuning constant (belongs to an owner, e.g. timeouts, thresholds), version identity, or other
its status: live (read, and the value matters), dead (never read), never changed (always the same value in every build), or bench-only

4. Overlaps. Anything defined or set in more than one place, especially the version identity (number, string, release notes) and log levels.

5. One-page summary:

counts by kind and status
the worst clutter (dead switches, duplicate definitions, settings nobody changes)
a proposed end state of about three files: (1) the build profile, with release and bench profiles and every build switch; (2) the version, number and string together (note whether the release notes should move to CHANGELOG.md, out of the binary); (3) tuning constants left with their owners, listed only
which current files would be merged, which deleted, and which kept

Report the model and reasoning level used. No edits.
