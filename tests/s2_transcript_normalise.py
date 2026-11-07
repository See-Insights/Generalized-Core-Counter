"""v50 S2 (WO-2026-11-05-004, Amendment 4 F6): maps the BASE sleep-execution transcript to its S2 form.

Only the approved S2 substitutions are applied, to the base transcript (the current transcript is never rewritten):
  - the AB1805 stale-interrupt clear and interruptAtTime() pre-arm events are gone;
  - the AB1805 watchdog stop/resume events become ExternalWatchdog stop/start;
  - the HIBERNATE sleep call gains duration(wakeInSeconds * 1000) (taken from the scenario's HIB armForSleep event);
  - the two AB1805 bypass warnings are gone (their branches no longer exist).
The per-platform DISTINCT_EVENTS count is recomputed from the normalised EVENT list.
usage: s2_transcript_normalise.py <base transcript>   (writes the normalised transcript to stdout)
"""
import re
import sys

DROP = ("HIB clearStale", "AB1805 interruptAtTime",
        "LOG WARN HIBERNATE bypass: failed stale interrupt cleanup", "LOG WARN HIBERNATE bypass: failed to program AB1805 alarm")


def event(line, wake_in):
    body = line.strip()
    if body.startswith(DROP):
        return None
    line = line.replace("AB1805 stopWDT", "EXTWDT stop").replace("AB1805 resumeWDT", "EXTWDT start")
    if "SLEEP mode=HIBERNATE" in line:
        if "dur=none" not in line or wake_in is None:
            sys.exit("S2 normalise: unexpected HIBERNATE sleep line in the base transcript: " + line)
        line = line.replace("dur=none", f"dur={wake_in * 1000}")
    return line


def segment(lines):
    out, events, summary, wake_in = [], [], None, None
    for line in lines:
        m = re.match(r"\s+HIB armForSleep \S+ (\d+)$", line)
        if m:
            wake_in = int(m.group(1))
        if line.startswith("SCENARIOS "):
            summary = len(out)
            out.append(line)
        elif line.startswith("EVENT "):
            events.append(line)
        else:
            if line.startswith("S") and re.match(r"S\d+ ", line):
                wake_in = None
            line = event(line, wake_in)
            if line is not None:
                out.append(line)
    # EVENT lines have their digit runs collapsed to '#', so the substitutions are matched in that form.
    digits = lambda t: re.sub(r"\d+", "#", t)
    drop = tuple(digits(d) for d in DROP)
    norm = set()
    for e in events:
        n = digits(e)
        if n[6:].startswith(drop):
            continue
        n = n.replace("AB# stopWDT", "EXTWDT stop").replace("AB# resumeWDT", "EXTWDT start")
        if "mode=HIBERNATE" in n:
            n = n.replace("dur=none -> ", "dur=# -> ")
        norm.add(n)
    norm = sorted(norm)
    if summary is None:
        sys.exit("S2 normalise: no summary line in a platform segment")
    out[summary] = re.sub(r"DISTINCT_EVENTS \d+", f"DISTINCT_EVENTS {len(norm)}", out[summary])
    return out + norm


def main(text):
    lines = text.split("\n")
    trailing = lines[-1] == ""
    if trailing:
        lines = lines[:-1]
    result, cur = [], []
    for line in lines:
        if line.startswith("PLATFORM ") and cur:
            result += segment(cur)
            cur = []
        cur.append(line)
    result += segment(cur)
    return "\n".join(result) + ("\n" if trailing else "")


if __name__ == "__main__":
    sys.stdout.write(main(open(sys.argv[1]).read()))
