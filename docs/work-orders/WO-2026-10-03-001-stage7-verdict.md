**Overall: VERIFIED WITH NOTES.** No production defect found within the approved acceptance criteria. Model/reasoning used: **gpt-6-astra, high**.

| Check | Verdict | Evidence |
|---|---|---|
| 1. No retry in flight | **PASS — VERIFIED** | Both guards match `existing && inflightForPointer`. STATUS and both DATA caller scenarios each ran 13,637 passes over a simulated 300-second wait: zero `set()` calls, payload builds, log lines, or allocations per pass. First pass after simulated completion wrote the latest content exactly once. Removing either guard failed. |
| 2. No drops | **PASS — VERIFIED** | Refused ReportState DATA, ConnectState DATA, and ConnectState STATUS writes drained after completion. DATA refusal still returns `true`; ReportState and alert-42 code are unchanged. Removing either deferral assignment failed. |
| 3. Clear-flag edit | **PASS — VERIFIED** | Successful direct DATA write after deferral produced no subsequent write after completion. Removing the success-branch clear failed the new regression test. |
| 4. One operation per pass | **PASS — VERIFIED** | With both writes deferred, the first pass wrote STATUS only. Removing the intervening `return` failed. |
| 5. Callback context | **PASS — VERIFIED** | Both callbacks are unchanged. They clear pending-sync flags and retain existing completion bookkeeping; neither writes or drains deferred work. |
| 6. Sleep gate | **PASS — VERIFIED** | `LedgerClient.cpp` and `State_Sleep.cpp` are byte-identical to v34. `pendingDataPublish` is excluded. The existing `pendingStatusPublish` term now keeps a refused ConnectState STATUS write waiting through its deferred round trip, as the WO notes. |
| 7. Test intent | **PASS — VERIFIED WITH NOTES** | Shim initialization preserves member order and refusal-path coverage; its always-zero stub remains appropriate. The pinned `Cloud.cpp:703` reference is correct. The stale power-source comment should be updated to describe historical behavior. |
| 8. Suite/build | **PASS — VERIFIED** | **61/61:** 32 shell tests using zsh, 29 Python tests using python3. Both protected structural tests are unchanged and green. Clean Boron release build succeeded after `clean-user`, using copied Device OS 6.4.1 and fresh object outputs. |
| 9. Budget | **PASS — VERIFIED** | **14 net lines:** Cloud.cpp +9, Cloud.h +1, DeviceStatusPublisher.cpp +4; version changes net zero. Counted physical nonblank, non-comment lines, including braces and preprocessor lines. No compression. |

Build sizes:

| | text | data | bss |
|---|---:|---:|---:|
| v34 | 150556 | 1090 | 2180 |
| Reviewed v35 | **150684** | **1090** | **2180** |
| Delta | **+128** | **0** | **0** |

This is **+8 text** versus Copilot’s pre-clear-flag build. `strings` finds `v35-LedgerNoRetry`; the compiled product-version field contains `0x0023` (**35**). Build warnings occur only in unchanged sources.

All **six mutations failed assertions**, and restored sources passed again.

The stale comment’s smallest correction is to replace its present-tense production comparison with: “This mirrors the historical production retry pattern fixed by WO-2026-10-03-001.”

The clear-flag regression belongs in the permanent suite. This exact proposed addition was tested successfully, and removing the clear line makes it fail. **Not applied:**

```diff
--- a/tests/ledger_no_retry_test.cpp
+++ b/tests/ledger_no_retry_test.cpp
@@ -304,6 +304,27 @@
   completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
 }
 
+// A successful direct DATA write supersedes an earlier deferred write.
+void testDirectDataWriteClearsDeferral(void *dataLedger) {
+  assert(Cloud::instance().publishDataToLedger("ConnectState"));
+  const int beforeRefusal = g_ledgerSetObserver.callCount;
+  assert(Cloud::instance().publishDataToLedger("ReportState"));
+  assert(g_ledgerSetObserver.callCount == beforeRefusal);
+  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
+
+  // A direct caller wins before loop() has drained the deferral.
+  testCurrent.totalOccupiedSecondsValue = 7654;
+  assert(Cloud::instance().publishDataToLedger("ReportState"));
+  assert(g_ledgerSetObserver.callCount == beforeRefusal + 1);
+  assert(g_ledgerSetObserver.lastPayload.find("totalOccupiedSec=7654;") != std::string::npos);
+  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
+
+  for (int i = 0; i < 50; ++i) {
+    advanceAndLoop();
+    assert(g_ledgerSetObserver.callCount == beforeRefusal + 1);
+  }
+}
+
 } // namespace
 
 int main() {
@@ -311,6 +332,7 @@
   void *dataLedger = testRefusedDataWriteIsWrittenAfterCompletion();
   testRefusedConnectStateStatusWriteIsWrittenAfterCompletion(statusLedger);
   testOneDeferredOperationPerPass(statusLedger, dataLedger);
+  testDirectDataWriteClearsDeferral(dataLedger);
 
   printf("ledger_no_retry_test: all assertions passed\n");
   return 0;
```

**Preservation confirmed:** the final SHA-256/path manifest matches all **125,214** initial repository entries, including ignored/untracked files and `.git`. The working tree is byte-identical. All task-created/deleted files stayed within `build-tmp/wo20261003-001-stage7/`, which was removed. `build-tmp/`, `connectivity-archive/`, and the unrelated recovery-plan edit remain intact. No network, AWS, device, or Git mutation actions were performed.