#!/usr/bin/env python3
"""
Generate a compact "naked eye" star map CSV from HYG v4.2 (hygdata_v42.csv.gz).

No external dependencies (no requests). Uses urllib + gzip + csv.

Input:
  https://www.astronexus.com/downloads/catalogs/hygdata_v42.csv.gz

Output CSV columns (minimal for Unreal runtime):
  id,proper,bf,ra,dec,mag,ci,dist,x,y,z,spect,con

Notes:
- mag <= 6.0 approximates naked-eye visibility under dark skies.
- HYG columns can contain blanks; we drop rows missing RA/Dec or magnitude.
"""

from __future__ import annotations

import argparse
import csv
import gzip
import io
import math
import sys
import urllib.request
from typing import Dict, Any, Optional


DEFAULT_URL = "https://www.astronexus.com/downloads/catalogs/hygdata_v42.csv.gz"


def as_float(s: str, default: float = float("nan")) -> float:
    if s is None:
        return default
    s = s.strip()
    if not s:
        return default
    try:
        return float(s)
    except ValueError:
        return default


def as_str(s: str, default: str = "") -> str:
    if s is None:
        return default
    s = str(s)
    return s.strip() if s.strip() else default


def pick_name(row: Dict[str, str]) -> str:
    # Prefer "proper" (IAU/common name), then Bayer/Flamsteed "bf"
    proper = as_str(row.get("proper", ""))
    bf = as_str(row.get("bf", ""))
    return proper or bf or ""


def download_gz_bytes(url: str, timeout_s: int) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "UETPFCore-StarGen/1.0"})
    with urllib.request.urlopen(req, timeout=timeout_s) as resp:
        data = resp.read()
    return data


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--url", default=DEFAULT_URL, help="HYG v4.2 gzip CSV URL")
    ap.add_argument("--out", required=True, help="Output CSV path")
    ap.add_argument("--mag-max", type=float, default=6.0, help="Max apparent magnitude (naked eye ~= 6.0)")
    ap.add_argument("--timeout", type=int, default=60, help="Download timeout seconds")
    ap.add_argument("--max-rows", type=int, default=0, help="Optional limit for debugging (0 = no limit)")
    args = ap.parse_args()

    print(f"[stargen] downloading: {args.url}")
    gz_data = download_gz_bytes(args.url, timeout_s=args.timeout)
    print(f"[stargen] downloaded bytes: {len(gz_data):,}")

    # Decompress gzip -> text stream
    with gzip.GzipFile(fileobj=io.BytesIO(gz_data), mode="rb") as gz:
        text = gz.read().decode("utf-8", errors="replace")

    # Parse CSV
    reader = csv.DictReader(io.StringIO(text))
    if reader.fieldnames is None:
        print("[ERROR] CSV has no header row.", file=sys.stderr)
        return 1

    # Validate required columns exist (HYG v4.2 uses these names)
    required_cols = ["id", "ra", "dec", "mag"]
    missing = [c for c in required_cols if c not in reader.fieldnames]
    if missing:
        print(f"[ERROR] Missing expected columns: {missing}", file=sys.stderr)
        print(f"Fields found: {reader.fieldnames[:50]} ...", file=sys.stderr)
        return 1

    kept = 0
    dropped_missing = 0
    dropped_mag = 0
    total = 0

    # Write output
    out_cols = ["id", "name", "proper", "bf", "ra", "dec", "mag", "ci", "dist", "x", "y", "z", "spect", "con"]
    with open(args.out, "w", newline="", encoding="utf-8") as f_out:
        w = csv.writer(f_out)
        w.writerow(out_cols)

        for row in reader:
            total += 1
            if args.max_rows and total > args.max_rows:
                break

            ra = as_float(row.get("ra", ""))
            dec = as_float(row.get("dec", ""))
            mag = as_float(row.get("mag", ""))

            if math.isnan(ra) or math.isnan(dec) or math.isnan(mag):
                dropped_missing += 1
                continue

            if mag > args.mag_max:
                dropped_mag += 1
                continue

            sid = as_str(row.get("id", ""))
            proper = as_str(row.get("proper", ""))
            bf = as_str(row.get("bf", ""))
            name = pick_name(row)

            ci = as_float(row.get("ci", ""), default=0.0)
            dist = as_float(row.get("dist", ""), default=0.0)
            x = as_float(row.get("x", ""), default=0.0)
            y = as_float(row.get("y", ""), default=0.0)
            z = as_float(row.get("z", ""), default=0.0)
            spect = as_str(row.get("spect", ""))
            con = as_str(row.get("con", ""))

            if sid == "0" or proper == "Sol" or name == "Sol":
                continue
            
            w.writerow([
                sid,
                name,
                proper,
                bf,
                f"{ra:.8f}",
                f"{dec:.8f}",
                f"{mag:.3f}",
                f"{ci:.3f}",
                f"{dist:.6f}",
                f"{x:.6f}",
                f"{y:.6f}",
                f"{z:.6f}",
                spect,
                con,
            ])
            kept += 1

    print(f"[stargen] total={total:,} kept={kept:,} dropped_missing={dropped_missing:,} dropped_mag={dropped_mag:,}")
    if kept == 0:
        print("[ERROR] Output is empty. Likely causes:", file=sys.stderr)
        print("  - Column names differ from expected (id/ra/dec/mag)", file=sys.stderr)
        print("  - mag values not parseable", file=sys.stderr)
        print("  - mag-max too strict", file=sys.stderr)
        return 2

    print(f"[stargen] wrote: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
