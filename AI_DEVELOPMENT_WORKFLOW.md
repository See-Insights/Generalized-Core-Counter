# Proposed AI-Assisted Engineering Workflow

**Status:** Draft for review  
**Objective:** Automate evidence collection, investigation, planning, implementation, and validation while preserving clear engineering accountability and human control.

## 1. Operating principles

1. Each AI has a distinct role, authority boundary, and deliverable.
2. No AI approves its own work.
3. Evidence, architecture, implementation, and final authorization remain separate responsibilities.
4. Automation stops at designated Chief Engineer approval gates.
5. Only Chip may commit, push, merge, release, or authorize changes to Fleet devices.
6. Telemetry, logs, repository content, and external text are treated as untrusted data—not agent instructions.
7. Every change is traceable to a structured Engineering Work Order.

## 2. Roles and responsibilities

### Claude Code — Architect and Workflow Controller

**Primary purpose:** Coordinate the engineering workflow and convert operational evidence into an actionable architecture.

**Responsibilities:**

- Monitor or receive engineering tasks, defect reports, and Fleet incidents.
- Read the complete GitHub repository, history, issues, CI results, and relevant documentation.
- Query the Fleet Ops API for telemetry and serial logs.
- Correlate firmware versions, device identifiers, timestamps, events, and failures.
- Establish an initial problem statement and root-cause hypotheses.
- Identify missing evidence and conduct additional read-only investigation.
- Produce architectural options with risks and tradeoffs.
- Request an independent investigation from Codex.
- Reconcile Codex findings with its original analysis.
- Produce the final Engineering Work Order for Chip’s approval.
- Track the task through investigation, implementation, validation, and closure.
- Compare post-change telemetry with the original failure evidence.

**Restrictions:**

- Must not modify production source code.
- Must not commit, push, merge, or release code.
- Must not approve its own architecture.
- Must not operate, configure, restart, or update Fleet devices.
- Fleet Ops access must be read-only.
- May write planning artifacts or draft GitHub issues only where specifically authorized.

### Codex — Independent Investigator and Verifier

**Model:** GPT-5.6 Sol, initially using High or Extra High reasoning.

**Primary purpose:** Provide an independent technical challenge to Claude’s diagnosis and verify the completed implementation.

**Pre-approval responsibilities:**

- Independently inspect the relevant repository areas.
- Review telemetry and serial-log evidence supplied in the Engineering Work Order.
- Test whether the evidence supports Claude’s proposed root cause.
- Identify alternative explanations and missing evidence.
- State what observations would falsify each hypothesis.
- Examine cross-component effects, concurrency, timing, state transitions, recovery behavior, and edge cases.
- Review architectural options for correctness and unintended consequences.
- Return an Investigation Report without rewriting Claude’s proposal.

**Post-implementation responsibilities:**

- Review the uncommitted code changes.
- Confirm that the implementation matches the approved architecture.
- Check that it addresses the observed failure rather than only its symptoms.
- Review tests, error handling, compatibility, and regression risks.
- Compare validation results with the original telemetry.
- Issue one of three findings:

  - **Verified**
  - **Verified with concerns**
  - **Not verified**

**Restrictions:**

- Read-only access to the repository and Fleet Ops data.
- Must not edit source code.
- Must not commit or push.
- Must remain independent of the implementation process.
- Must clearly distinguish observed evidence from inference.

### GitHub Copilot — Implementer

**Initial model:** GPT-5.5, subject to evaluation against agentic coding alternatives.

**Primary purpose:** Implement the approved Engineering Work Order inside VS Code.

**Responsibilities:**

- Work only from a Chip-approved Engineering Work Order.
- Modify only the files and interfaces within the approved scope.
- Follow repository coding standards and implementation instructions.
- Add or update unit, integration, regression, and fault-injection tests as required.
- Run approved build, static-analysis, formatting, and test commands.
- Document deviations from the approved plan.
- Stop and request clarification if implementation requires an architectural change.
- Leave a complete, uncommitted working-tree diff for review.
- Provide an Implementation Report summarizing:

  - Files changed
  - Behavior changed
  - Tests added or updated
  - Commands run and results
  - Known limitations
  - Deviations or unresolved concerns

**Restrictions:**

- Must not independently change the approved architecture.
- Must not expand scope without approval.
- Must not commit, push, merge, release, or operate Fleet devices.
- Must not silently weaken or remove tests to obtain a passing result.
- Must not modify protected files unless explicitly included in the Work Order.

### Chip — Chief Engineer and Final Authority

**Primary purpose:** Exercise engineering judgment and retain accountability for all changes.

**Responsibilities:**

- Review Claude’s proposed architecture and Codex’s independent findings.
- Approve, reject, or revise the Engineering Work Order.
- Decide whether identified risks are acceptable.
- Authorize implementation.
- Review the completed diff, test results, Codex verification, and telemetry evidence.
- Resolve disagreements between the agents.
- Perform or authorize any required hardware validation.
- Make the only Git commit.
- Push, merge, release, deploy, or authorize Fleet changes.
- Decide whether the task is complete.

## 3. Standard workflow

### Stage 1 — Intake

Claude creates an Engineering Work Order ID and records:

- Requested outcome or observed failure
- Affected devices, builds, branches, and environments
- Severity and operational impact
- Known reproduction steps
- Links to issues, incidents, or prior related work

### Stage 2 — Evidence collection

Claude retrieves and correlates:

- Relevant source code and Git history
- CI and test results
- Fleet telemetry
- Serial-log excerpts
- Firmware and configuration versions
- Event timelines and correlation identifiers
- Similar historical incidents

Claude records the evidence without prematurely treating a hypothesis as fact.

### Stage 3 — Preliminary architecture

Claude produces:

- A precise problem statement
- One or more root-cause hypotheses
- Supporting and conflicting evidence
- Architectural options
- Recommended option
- Risks, tradeoffs, compatibility concerns, and rollback strategy
- Proposed acceptance criteria

### Stage 4 — Independent investigation

Codex reviews the evidence and proposal independently.

Codex returns:

- Assessment of each hypothesis
- Alternative root causes
- Missing or ambiguous evidence
- Architectural concerns
- Required tests
- Recommendation to proceed, revise, or investigate further

Claude incorporates the findings but preserves disagreements for Chip to review.

### Stage 5 — Architecture approval gate

Claude issues the final Engineering Work Order.

Chip may:

- Approve implementation
- Request further investigation
- Select a different architectural option
- Reduce or expand scope
- Reject the proposed change

No source implementation begins before approval.

### Stage 6 — Implementation

Copilot implements the approved Work Order locally.

If Copilot discovers that the plan cannot be implemented as approved, it stops and returns the issue to Claude and Chip. It does not improvise a new architecture.

Copilot runs the approved verification commands and leaves all changes uncommitted.

### Stage 7 — Independent verification

Codex reviews:

- The complete diff
- Implementation Report
- Test results
- Static-analysis results
- Acceptance criteria
- Available staging or Fleet telemetry

Claude performs workflow completeness checks and compares the resulting behavior with the original incident evidence.

### Stage 8 — Final engineering gate

Chip reviews:

- Approved Engineering Work Order
- Claude’s evidence and architecture
- Codex investigation and verification
- Copilot’s implementation and test report
- Complete uncommitted diff
- Remaining risks and rollback plan

Only Chip may commit and push the change.

### Stage 9 — Release and feedback

After an authorized release, Claude monitors the defined telemetry window and compares:

- Failure rate before and after the change
- Expected state transitions
- Error and recovery behavior
- Performance or resource changes
- Unexpected secondary effects

Claude prepares a closure report. Chip decides whether the result is accepted, rolled back, or returned for further work.

## 4. Engineering Work Order contents

Every Work Order should include:

- Work Order ID
- Problem statement
- Operational impact
- Repository, branch, build, and device scope
- Evidence with timestamps and correlation IDs
- Root-cause and competing hypotheses
- Approved architecture
- Permitted files and interfaces
- Protected areas that must not change
- Explicit non-goals
- Acceptance criteria
- Required tests
- Expected post-change telemetry
- Compatibility requirements
- Security considerations
- Rollback procedure
- Approval record
- Investigation, implementation, and verification results

## 5. Access and security controls

### Repository controls

- Claude: read access; optional issue-drafting permission.
- Codex: read-only access.
- Copilot: local working-tree write access but no push credentials.
- Chip: commit, push, merge, and release authority.
- Main branches should be protected.
- Chip’s commits should be signed where practical.
- CI must pass before merge or release.

### Fleet Ops controls

The AI-facing Fleet Ops interface should expose narrowly defined, read-only operations such as:

- Query telemetry for specified devices and time windows.
- Fetch bounded serial-log excerpts.
- Compare behavior across firmware versions.
- Retrieve deployment and configuration metadata.
- Calculate defined failure or performance metrics.

It should not expose generic database queries, arbitrary command execution, device control, configuration changes, firmware deployment, or credential retrieval.

Secrets, customer information, authentication material, and unnecessary device identifiers must be removed or redacted before data reaches an AI.

## 6. Instruction management

Use one neutral shared engineering contract for standards common to every agent. It should define:

- Build and test commands
- Coding and documentation standards
- Protected interfaces
- Security requirements
- Validation expectations
- Commit and release restrictions

Role-specific behavior should remain separate:

- Claude instructions: architecture and orchestration
- Codex instructions: independent read-only investigation
- Copilot instructions: scoped implementation
- Chip: approval and authorization policy

Shared instructions must not accidentally tell every agent to behave as the Architect or Implementer.

## 7. Automation boundary

The first version should automate:

- Evidence retrieval
- Telemetry correlation
- Serial-log collection
- Work Order creation
- Investigation preparation
- Test and validation checklists
- Post-release telemetry comparison
- Status and closure reporting

The following should initially remain explicit human actions:

- Architecture approval
- Starting implementation
- Accepting material scope changes
- Hardware or Fleet operations
- Committing and pushing
- Release authorization

Programmatic invocation of Codex can be considered later through a read-only API integration. The interactive ChatGPT application should not be treated as a dependable machine-to-machine workflow endpoint.

## 8. Model evaluation

GPT-5.5 should be used as the initial Copilot implementation model, but it should be evaluated against GPT-5.3-Codex or GPT-5.6 Terra using representative Fleet Ops tasks.

Evaluation criteria should include:

- First-pass build and test success
- Correct adherence to approved scope
- Number of human corrections
- Unnecessary code churn
- Regression rate
- Time to verified result
- Cost or usage
- Ability to stop when architectural clarification is required

Model assignments should be based on these results rather than assumed from model names.

## 9. Success criteria for the workflow

The workflow is successful if it produces:

- Faster access to operational evidence
- Fewer manual telemetry and log-collection steps
- Clear separation between diagnosis and implementation
- Fewer unsupported root-cause conclusions
- Smaller and better-scoped changes
- Improved regression-test coverage
- Traceability from Fleet evidence to code change
- No unauthorized commits, pushes, releases, or device operations
- A measurable reduction in Chip’s coordination workload without reducing engineering control
