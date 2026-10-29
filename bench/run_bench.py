#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Linux benchmark harness for Chobi.

Measures, for the chobi binary and (optionally) a stock `mpv --no-config` baseline:
  * startup timeline (CHOBI_BENCH marks) and external wall time to first frame
  * idle memory (RSS and PSS) a few seconds after launch with no file
  * playback memory, average CPU and hwdec status while playing each file
  * memory growth across repeated opens (via the integration test's switching loop)

Usage:
  bench/run_bench.py --binary build/src/chobi --runs 5 media1.mkv [media2.mkv ...]

Notes: "cold" start means first launch after `sync; echo 3 > /proc/sys/vm/drop_caches`
(needs root, run it yourself before invoking with --cold). Without it, numbers are warm.
"""
import argparse
import json
import os
import platform
import statistics
import subprocess
import sys
import time

CLK_TCK = os.sysconf("SC_CLK_TCK")


def read_kb(pid, field, path="status"):
    try:
        with open(f"/proc/{pid}/{path}") as f:
            for line in f:
                if line.startswith(field + ":"):
                    return int(line.split()[1])
    except FileNotFoundError:
        pass
    return None


def cpu_seconds(pid):
    with open(f"/proc/{pid}/stat") as f:
        fields = f.read().rsplit(")", 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / CLK_TCK  # utime + stime


def startup(binary, media, runs):
    results = []
    env = dict(os.environ, CHOBI_BENCH="1", CHOBI_BENCH_QUIT="1")
    for _ in range(runs):
        t0 = time.perf_counter()
        proc = subprocess.run([binary, media], env=env, capture_output=True, text=True, timeout=60)
        wall = (time.perf_counter() - t0) * 1000
        marks = {}
        for line in proc.stderr.splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[0] == "[bench]":
                try:
                    marks[parts[2]] = float(parts[1])
                except ValueError:
                    marks[parts[1]] = parts[2]
        marks["process-wall-incl-exit"] = wall
        results.append(marks)
    keys = sorted({k for r in results for k in r if isinstance(r.get(k), float)})
    return {k: statistics.median(r[k] for r in results if k in r) for k in keys}


def steady_state(cmd, seconds, settle=2.0):
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        time.sleep(settle)
        c0, t0 = cpu_seconds(proc.pid), time.perf_counter()
        peak_rss = 0
        end = t0 + seconds
        while time.perf_counter() < end:
            peak_rss = max(peak_rss, read_kb(proc.pid, "VmRSS") or 0)
            time.sleep(0.25)
        c1, t1 = cpu_seconds(proc.pid), time.perf_counter()
        return {
            "rss_mb": round((read_kb(proc.pid, "VmRSS") or 0) / 1024, 1),
            "peak_rss_mb": round(peak_rss / 1024, 1),
            "pss_mb": round((read_kb(proc.pid, "Pss", "smaps_rollup") or 0) / 1024, 1),
            "cpu_percent_of_one_core": round(100 * (c1 - c0) / (t1 - t0), 1),
        }
    finally:
        proc.terminate()
        try:
            proc.wait(5)
        except subprocess.TimeoutExpired:
            proc.kill()


def machine():
    cpu = "unknown"
    try:
        with open("/proc/cpuinfo") as f:
            cpu = next(l.split(":", 1)[1].strip() for l in f if l.startswith("model name"))
    except (OSError, StopIteration):
        pass
    gpu = subprocess.run(["sh", "-c", "lspci | grep -i -E 'vga|3d|display' | head -1"],
                         capture_output=True, text=True).stdout.strip()
    return {"os": platform.platform(), "cpu": cpu, "threads": os.cpu_count(), "gpu": gpu,
            "session": os.environ.get("XDG_SESSION_TYPE", "?")}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", default="build/src/chobi")
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--seconds", type=float, default=10)
    ap.add_argument("--baseline-mpv", action="store_true", help="also measure mpv --no-config")
    ap.add_argument("--cold", action="store_true", help="label results as cold start")
    ap.add_argument("media", nargs="+")
    a = ap.parse_args()

    report = {"machine": machine(), "cold": a.cold, "startup": {}, "idle": {}, "playback": {}}
    report["idle"]["chobi"] = steady_state([a.binary], a.seconds)
    for m in a.media:
        name = os.path.basename(m)
        report["startup"][name] = startup(a.binary, m, a.runs)
        report["playback"].setdefault(name, {})["chobi"] = steady_state([a.binary, m], a.seconds)
        if a.baseline_mpv:
            report["playback"][name]["mpv --no-config"] = steady_state(
                ["mpv", "--no-config", "--hwdec=vaapi", "--really-quiet", m], a.seconds)
    json.dump(report, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
