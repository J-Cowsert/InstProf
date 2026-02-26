#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build" / "bench"
BIN_DIR = BUILD_DIR / "bench"
RESULTS_DIR = ROOT / "bench" / "results"


def build():
    print("=== Configuring ===")
    if subprocess.run(["cmake", "--preset", "bench"], cwd=ROOT).returncode != 0:
        sys.exit("Configure failed.")

    print("\n=== Building ===")
    if subprocess.run(["cmake", "--build", str(BUILD_DIR)], cwd=ROOT).returncode != 0:
        sys.exit("Build failed.")
    print()


def discover_benchmarks():
    if not BIN_DIR.exists():
        return []
    return sorted(
        p for p in BIN_DIR.iterdir()
        if p.is_file() and p.stem.startswith("bench_") and os.access(p, os.X_OK)
    )


def run_benchmarks():
    build()

    binaries = discover_benchmarks()
    if not binaries:
        sys.exit("No benchmark binaries found.")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y-%m-%d_%H%M%S")

    for binary in binaries:
        label = binary.stem.removeprefix("bench_")
        out_file = RESULTS_DIR / f"{timestamp}_{label}.json"

        print(f"=== Running {label} ===")
        r = subprocess.run([
            str(binary),
            f"--benchmark_out={out_file}",
            "--benchmark_out_format=json",
        ])

        if r.returncode != 0:
            print(f"{label} failed.")
            continue

        print(f"Saved: {out_file.relative_to(ROOT)}\n")


def main():
    parser = argparse.ArgumentParser(description="InstProf benchmark runner")
    sub = parser.add_subparsers(dest="command")
    sub.add_parser("run", help="Build and run benchmarks")

    args = parser.parse_args()
    if args.command == "run":
        run_benchmarks()
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
