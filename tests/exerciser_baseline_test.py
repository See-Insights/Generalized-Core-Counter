#!/usr/bin/env python3
"""Known-answer tests for tools/exerciser_baseline.py (WO-2026-10-23-001); hook JSON is generated here.

Hand arithmetic, 60 s debounce: credit per session = 9*30 + 60 = 330 s = 5.5 min. Day 2026-10-21 UTC,
open 7, close 10. T0 = 07:54:30, so session k closes at 08:00:00 + 20 min * k; k = 0..5 close by 09:40,
k = 6 closes at exactly 10:00:00, which is NOT before the close hour: 6 sessions, expected 33.0 min.
A report follows each close by 60 s, then a close report at 10:00:30; dailyoccupancy = floor(minutes).
- perfect: 5.5*k truncated = 5, 11, 16, 22, 27, 33. Day 33 vs 33.0, error 0. Windows (7, last one
  empty): 5, 6, 5, 6, 5, 6, 0 vs 5.5 each: errors -.5, +.5, -.5, +.5, -.5, +.5, 0.
- plus60: 6.5 min per session: 6, 13, 19, 26, 32, 39. Day error +6.0 min = 18.1818%. Window errors
  +.5, +1.5, +.5, +1.5, +.5, +1.5, 0.
- missing: the report after close 2 is dropped. Day still 33, flagged; windows 6; the merged window
  measures 22 - 11 = 11 vs expected 2 * 5.5 = 11.
- SGT-8 (UTC+8, midnight 10-20 16:00Z) with open 7, and EST5EDT (UTC-4 in October, midnight 04:00Z) with
  open 20, close 23 (T0 20:54:30 local = 00:54:30Z next day): same offsets from local midnight, so the same
  answers as perfect. A wrong sign or UTC-day assignment finds no report that day.
- no_ts: an extra occupancy=1 report 30 min before T0 with no payload timestamp (published_at would be
  T0 - 1400 s). It must be ignored and noted; the answers equal perfect.
- reset: session 2 (08:34:30..08:40:00) is cut by a reset at 08:37 and not credited: 5, 11, 11, 16, 22, 27.
  Day 27 vs 33.0, error -6.0 min = -18.1818%, flagged; its window measures 0 vs 5.5 = -5.5.
"""
import csv, json, os, shutil, subprocess, sys, tempfile
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, "..", "tools", "exerciser_baseline.py")
DEV = "e00fce68399ee6244a963935"
BASE = datetime(2026, 10, 21, tzinfo=timezone.utc).timestamp()


def iso(t):
    return datetime.fromtimestamp(t, timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.000Z")


def write(root, sub, name, ts, payload, pub_delay=400):
    d = os.path.join(root, sub)
    os.makedirs(d, exist_ok=True)
    env = {"particle": {"data": json.dumps(payload).replace('"', "&quot;"), "coreid": DEV,
                        "published_at": iso(ts + pub_delay), "fw_version": "45"}}
    with open(os.path.join(d, name + ".json"), "w") as f:
        json.dump(env, f)


def run(credit_s=330, drop=None, reset=False, base=BASE, tz="UTC", h=7, no_ts=False):
    tmp, T0 = tempfile.mkdtemp(), base + h * 3600 + 54 * 60 + 30
    # published_at lags the payload timestamp by 400 s, so using it as the phase moves every close
    reps, cum = [(T0, 1, 0)], 0
    for k in range(6):
        if not (reset and k == 2):
            cum += credit_s
        reps.append((T0 + 330 + 1200 * k + 60, 0, int(cum // 60)))
    reps.append((base + (h + 3) * 3600 + 30, 0, int(cum // 60)))
    if no_ts:
        write(tmp, "hooks", "nots", T0 - 1800, {"occupancy": 1, "dailyoccupancy": 0, "cyc": 0})
    for i, (ts, occ, daily) in enumerate(r for j, r in enumerate(reps) if j != drop):
        write(tmp, "hooks", f"h{i}", ts, {"occupancy": occ, "dailyoccupancy": daily, "cyc": i,
                                           "timestamp": int(ts * 1000)}, 400)
    if reset:
        write(tmp, "status", "s0", base + (h + 1) * 3600 + 37 * 60, {"resetReason": 60}, 0)
    out = os.path.join(tmp, "out.csv")
    p = subprocess.run([sys.executable, SCRIPT, "--hooks", os.path.join(tmp, "hooks"), "--status",
                        os.path.join(tmp, "status"), "--id", DEV, "--dates", "2026-10-21", "--open", str(h),
                        "--close", str(h + 3), "--tz", tz, "--debounce-ms", "60000", "--csv", out],
                       capture_output=True, text=True, check=True)
    rows = list(csv.DictReader(open(out)))
    shutil.rmtree(tmp)
    day = next(r for r in rows if r["kind"] == "day")
    return p.stdout, day, [float(r["error"]) for r in rows if r["kind"] == "window"]


def check(name, got, want):
    if got != want:
        print(f"FAIL {name}: got {got!r}, want {want!r}")
        return 1
    return 0


def main():
    bad = 0
    out, day, win = run()
    bad += check("perfect day", (day["measured"], day["expected"], float(day["error"]), day["count"], day["flags"]),
                 ("33", "33.0", 0.0, "6", ""))
    bad += check("perfect windows", win, [-.5, .5, -.5, .5, -.5, .5, 0.0])
    bad += check("truncation note", "truncated to whole minutes" in out and "+/-1 min" in out, True)
    out, day, win = run(credit_s=390)
    bad += check("plus60 day", (day["measured"], float(day["error"]), day["error_pct"]), ("39", 6.0, "18.1818"))
    bad += check("plus60 windows", win, [.5, 1.5, .5, 1.5, .5, 1.5, 0.0])
    out, day, win = run(drop=3)
    bad += check("missing day", (day["measured"], float(day["error"]), day["flags"]), ("33", 0.0, "missing-reports"))
    bad += check("missing windows", (len(win), win[2]), (6, 0.0))
    out, day, win = run(reset=True)
    bad += check("reset day", (day["measured"], float(day["error"]), day["error_pct"], day["flags"]),
                 ("27", -6.0, "-18.1818", "reset"))
    bad += check("reset window", win[2], -5.5)
    # SGT-8 = UTC+8: local midnight is 2026-10-20 16:00Z, so the 07:54 T0 is on the previous UTC date
    for tz, b, h in (("SGT-8", BASE - 8 * 3600, 7), ("EST5EDT", BASE + 4 * 3600, 20)):
        out, day, win = run(base=b, tz=tz, h=h)
        bad += check(tz + " day", (day["measured"], day["expected"], day["count"]), ("33", "33.0", "6"))
        bad += check(tz + " windows", win, [-.5, .5, -.5, .5, -.5, .5, 0.0])
    out, day, win = run(no_ts=True)
    bad += check("no-timestamp ignored", (day["measured"], day["count"], win, "1 reports without a payload timestamp ignored" in out),
                 ("33", "6", [-.5, .5, -.5, .5, -.5, .5, 0.0], True))
    print("exerciser_baseline_test:", "FAIL" if bad else "PASS")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
