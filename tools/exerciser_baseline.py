#!/usr/bin/env python3
"""Exerciser baseline: occupied minutes a counter reported vs what the V1 pattern should give.

Reads local S3 report-hook and status JSON files only (no network, no AWS).
"""
import argparse, csv, glob, html, json, os, re, statistics, sys
from datetime import date, datetime, time, timedelta, timezone
from zoneinfo import ZoneInfo, ZoneInfoNotFoundError

TRUNC_NOTE = ("NOTE: dailyoccupancy is truncated to whole minutes, so a +/-1 min error "
              "per window is quantization, not firmware.")
PHASE_NOTE = ("NOTE: T0 is the payload timestamp of the day's first occupancy=1 report. It carries the "
              "connect delay (seconds to minutes), so expected closes can sit late and move credit "
              "between windows; try --phase-offset-s. Expected minutes never come from the counter.")


def get_tz(name):
    try:
        return ZoneInfo(name)
    except (ZoneInfoNotFoundError, ValueError, OSError):
        m = re.fullmatch(r"[A-Za-z]{3,}([+-]?)(\d{1,2})(?::([0-5]\d))?", name)
        if not m:
            sys.exit(f"error: unknown timezone {name!r}; use an IANA name or a fixed POSIX offset like SGT-8")
        mins = int(m.group(2)) * 60 + int(m.group(3) or 0)
        # POSIX sign is inverted: SGT-8 is UTC+8, EST5 is UTC-5
        return timezone(timedelta(minutes=mins if m.group(1) == "-" else -mins), name)


def load(root, dev, pub_ok=False):
    out, skipped = [], 0
    for p in sorted(glob.glob(os.path.join(root, "**", "*.json"), recursive=True)):
        try:
            with open(p) as f:
                part = json.load(f)["particle"]
            data = json.loads(html.unescape(part["data"]))
            pub = datetime.fromisoformat(part["published_at"].replace("Z", "+00:00")).timestamp()
        except (OSError, ValueError, KeyError, TypeError):
            continue
        if part.get("coreid") != dev:
            continue
        ms = data.get("timestamp")
        if not isinstance(ms, (int, float)):
            if not pub_ok:
                skipped += 1
                continue
            ms = pub * 1000
        out.append(dict(ts=ms / 1000, fw=part.get("fw_version") or "?", d=data))
    return sorted(out, key=lambda r: r["ts"]), skipped


def phase(r):
    return r["ts"]


def mean_sd(xs):
    return (statistics.mean(xs), statistics.stdev(xs) if len(xs) > 1 else 0.0) if xs else (None, None)


def pct(err, exp):
    return err / exp * 100 if exp else None


def fmt(v, nd=2):
    return "-" if v is None else f"{v:.{nd}f}"


def parse_args(argv):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--hooks", required=True, help="dir of hook JSON files (searched recursively)")
    ap.add_argument("--status", help="dir of status JSON files, for resets")
    ap.add_argument("--id", required=True, help="device (core) id")
    ap.add_argument("--dates", nargs="+", required=True, help="local days, YYYY-MM-DD")
    ap.add_argument("--tz", default="UTC")
    ap.add_argument("--open", type=float, default=0, help="local open hour")
    ap.add_argument("--close", type=float, default=24, help="local close hour")
    ap.add_argument("--debounce-ms", type=int, required=True, help="300000 production, 60000 bench")
    ap.add_argument("--cycle-s", type=float, default=1200)
    ap.add_argument("--pulses", type=int, default=10)
    ap.add_argument("--spacing-s", type=float, default=30)
    ap.add_argument("--phase-offset-s", type=float, default=0, help="added to T0")
    ap.add_argument("--exclude", nargs="*", default=[], help="START..END local ISO datetimes")
    ap.add_argument("--csv", help="write rows to this file")
    return ap.parse_args(argv)


def sessions(t0, close, credit, cycle_s):
    out, k = [], 0
    while t0 + k * cycle_s + credit < close:
        out.append(t0 + k * cycle_s + credit)
        k += 1
    return out


def main(argv=None):
    a = parse_args(argv)
    tz = get_tz(a.tz)
    hooks, no_ts = load(a.hooks, a.id)
    resets = [s["ts"] for s in (load(a.status, a.id, pub_ok=True)[0] if a.status else [])
              if s["d"].get("resetReason") not in (None, 30, 70)]
    excl = []
    for e in a.exclude:
        lo, hi = (datetime.fromisoformat(x) for x in e.split(".."))
        excl.append((lo.replace(tzinfo=lo.tzinfo or tz).timestamp(), hi.replace(tzinfo=hi.tzinfo or tz).timestamp()))
    credit = (a.pulses - 1) * a.spacing_s + a.debounce_ms / 1000
    hhmm = lambda t: datetime.fromtimestamp(t, tz).strftime("%H:%M:%S")
    overlaps = lambda lo, hi: any(lo <= e1 and e0 <= hi for e0, e1 in excl)
    days, wins = [], []
    for day in a.dates:
        d0 = datetime.combine(date.fromisoformat(day), time(0), tz)
        open_t = (d0 + timedelta(hours=a.open)).timestamp()
        close_t = (d0 + timedelta(hours=a.close)).timestamp()
        reps = [r for r in hooks if d0.timestamp() <= r["ts"] < (d0 + timedelta(days=1)).timestamp()
                and r["d"].get("dailyoccupancy") is not None]
        first = next((r for r in reps if r["d"].get("occupancy") == 1 and r["ts"] >= open_t), None)
        if not first:
            print(f"{day}: no occupancy=1 report, skipped")
            continue
        closes = sessions(phase(first) + a.phase_offset_s, close_t, credit, a.cycle_s)
        gaps = [b["ts"] - p["ts"] for p, b in zip(reps, reps[1:])]
        flags, dwins = [], []
        if gaps and max(gaps) > 1.5 * statistics.median(gaps):
            flags.append("missing-reports")
        if overlaps(reps[0]["ts"], reps[-1]["ts"]):
            flags.append("excluded")
        for p, c in zip(reps, reps[1:]):
            wf = []
            if any(p["ts"] < t <= c["ts"] for t in resets) or c["d"].get("cyc", 0) < p["d"].get("cyc", 0):
                wf.append("reset")
            if c["ts"] - p["ts"] > 1.5 * statistics.median(gaps):
                wf.append("gap")
            if overlaps(p["ts"], c["ts"]):
                wf.append("excluded")
            m = c["d"]["dailyoccupancy"] - p["d"]["dailyoccupancy"]
            x = sum(credit for t in closes if p["ts"] < t <= c["ts"]) / 60
            dwins.append(dict(label=f"{day} {hhmm(p['ts'])}-{hhmm(c['ts'])}", fw=c["fw"], m=m, x=x,
                              e=m - x, flags=wf))
        if any("reset" in w["flags"] for w in dwins):
            flags.append("reset")
        measured, expected = reps[-1]["d"]["dailyoccupancy"], len(closes) * credit / 60
        wins += dwins
        days.append(dict(label=day, fw=reps[-1]["fw"], m=measured, x=expected, e=measured - expected,
                         n=len(closes), flags=flags))

    print(f"Exerciser baseline, device {a.id}: debounce {a.debounce_ms} ms, {credit / 60:.2f} min credit per session")
    print(PHASE_NOTE)
    print(TRUNC_NOTE)
    if no_ts:
        print(f"NOTE: {no_ts} reports without a payload timestamp ignored.")
    print("\nPER DAY (minutes; error = measured - expected)")
    print(f"{'day':<12}{'fw':<8}{'measured':>9}{'expected':>9}{'error':>8}{'err%':>8}{'sessions':>9}  flags")
    for r in days:
        print(f"{r['label']:<12}{r['fw']:<8}{r['m']:>9}{r['x']:>9.2f}{r['e']:>8.2f}"
              f"{fmt(pct(r['e'], r['x'])):>8}{r['n']:>9}  {','.join(r['flags'])}")
    print("\nPER WINDOW (minutes)")
    print(f"{'window':<30}{'fw':<8}{'measured':>9}{'expected':>9}{'error':>8}  flags")
    for r in wins:
        print(f"{r['label']:<30}{r['fw']:<8}{r['m']:>9}{r['x']:>9.2f}{r['e']:>8.2f}  {','.join(r['flags'])}")
    print("\nSUMMARY per fw_version (excluded days/windows left out; sample standard deviation)")
    print(f"{'fw':<8}{'days':>5}{'acc(mean)':>10}{'prec(sd)':>9}{'windows':>9}{'w.mean':>8}{'w.sd':>8}")
    summary = []
    for fw in sorted({r["fw"] for r in days + wins}):
        de = [r["e"] for r in days if r["fw"] == fw and "excluded" not in r["flags"]]
        we = [r["e"] for r in wins if r["fw"] == fw and "excluded" not in r["flags"]]
        (dm, ds), (wm, ws) = mean_sd(de), mean_sd(we)
        summary.append((fw, len(de), dm, ds, len(we), wm, ws))
        print(f"{fw:<8}{len(de):>5}{fmt(dm):>10}{fmt(ds):>9}{len(we):>9}{fmt(wm):>8}{fmt(ws):>8}")

    if a.csv:
        with open(a.csv, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["kind", "label", "fw", "measured", "expected", "error", "error_pct", "count", "stdev", "flags"])
            for kind, rows in (("day", days), ("window", wins)):
                for r in rows:
                    w.writerow([kind, r["label"], r["fw"], r["m"], round(r["x"], 4), round(r["e"], 4),
                                fmt(pct(r["e"], r["x"]), 4) if kind == "day" else "", r.get("n", ""),
                                "", " ".join(r["flags"])])
            for fw, nd, dm, ds, nw, wm, ws in summary:
                w.writerow(["summary-day", fw, fw, "", "", fmt(dm, 4), "", nd, fmt(ds, 4), ""])
                w.writerow(["summary-window", fw, fw, "", "", fmt(wm, 4), "", nw, fmt(ws, 4), ""])
    return 0


if __name__ == "__main__":
    sys.exit(main())
