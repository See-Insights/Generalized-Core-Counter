# GitHub Issue Drafts

Ready-to-use small, actionable issues for the stability & maintainability work.

---

### Issue 1: Document persistent data versioning and migration rules

**Problem**  
Persistent data structure (`MyPersistentData.h/cpp`) evolves over time. v14 changes make it important to clearly document versioning.

**Why it matters**  
Prevents data corruption or loss of configuration when devices upgrade in the field.

**Acceptance Criteria**
- Add clear notes on current persistent data version.
- Document migration rules from previous versions (especially v13 → v14).
- Reference how `StorageHelperRK` handles layout changes.

**Suggested files to update**
- `docs/architecture-overview.md` or create `docs/persistent-data.md`
- `src/MyPersistentData.h` (header comments)

---

### Issue 2: Add release consistency checklist for Particle integer firmware versions

**Problem**  
Version numbers can get out of sync between source, README, CHANGELOG, and Particle console.

**Why it matters**  
Avoids confusion during OTA, debugging, and fleet management.

**Acceptance Criteria**
- Create or update a short release checklist.
- Verify PRODUCT_VERSION / FIRMWARE_VERSION consistency before tagging releases.

**Suggested files to update**
- `README.md`
- `CHANGELOG.md`
- `RELEASE_NOTES_*.md` (or similar)

---

### Issue 3: Document Boron vs Photon 2 sleep and reset behavior

**Problem**  
Boron and Photon 2 handle sleep, hibernate, and wake/reset differently. This is not clearly documented for field operators.

**Why it matters**  
Reduces confusion when troubleshooting field devices (especially "unexpected" resets on Photon 2).

**Acceptance Criteria**
- Document ULP sleep (Boron), HIBERNATE wake (Photon 2), and expected reset reasons.
- Add troubleshooting notes for common wake/reset patterns.

**Suggested files to update**
- `README.md` (new troubleshooting section)
- `docs/recovery-architecture.md`

---

### Issue 4: Add lightweight build verification instructions to README

**Problem**  
Team members need a quick, reliable way to verify they can build the production firmware.

**Why it matters**  
Reduces "works on my machine" issues and ensures consistent builds.

**Acceptance Criteria**
- Add simple build command(s) for production (no local paths).
- Mention required PlatformIO / Particle CLI setup.

**Suggested files to update**
- `README.md`

---

### Issue 5: Run and document local secret scan before v14 field rollout

**Problem**  
Risk of accidentally committing credentials or sensitive local data.

**Why it matters**  
Protects device fleet security and prevents exposure of Wi-Fi/Particle credentials.

**Acceptance Criteria**
- Run a secret scan (GitHub secret scanner, trufflehog, or `git grep`).
- Document the result (clean or any findings) in release notes.

**Suggested files to update**
- `README.md` (add to release process section)
- Release notes for v14