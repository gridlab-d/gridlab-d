#!/usr/bin/env python3
"""Scan Markdown docs for figures/tables missing style-guide captions.

Rules checked:
- Figure caption present only when image line matches:
  ![Caption](path){ #fig:anchor }
- Table caption present only when the nearest previous non-empty line before a
  Markdown table is:
  Table: Caption { #tbl:anchor }

Notes:
- Ignores fenced code blocks using ``` or ~~~.
- Detects both direct image syntax and linked-image syntax [![...](...)] as
  figures that require caption formatting.
- Writes CSV report and prints summary counts.
"""

from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


FENCE_RE = re.compile(r"^\s*(```|~~~)")
DIRECT_IMAGE_RE = re.compile(r"^\s*!\[[^\]]*\]\([^\)]+\)")
LINKED_IMAGE_RE = re.compile(r"^\s*\[!\[[^\]]*\]\([^\)]+\)\]")
FIG_CAPTION_RE = re.compile(r"^\s*!\[[^\]]+\]\([^\)]+\)\s*\{\s*#fig:[^}]+\}\s*$")
TABLE_SEP_RE = re.compile(r"^\s*\|?\s*:?-{3,}[-| :]*\s*$")
TABLE_CAPTION_RE = re.compile(r"^\s*Table:\s+.+\{\s*#tbl:[^}]+\}\s*$")


@dataclass
class Finding:
    kind: str
    file: str
    line: int
    detail: str


def markdown_files(root: Path) -> Iterable[Path]:
    yield from root.rglob("*.md")


def is_image(line: str) -> bool:
    return bool(DIRECT_IMAGE_RE.match(line) or LINKED_IMAGE_RE.match(line))


def scan_file(path: Path, repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return findings

    in_fence = False

    for i, line in enumerate(lines):
        if FENCE_RE.match(line):
            in_fence = not in_fence
            continue

        if in_fence:
            continue

        if is_image(line):
            if not FIG_CAPTION_RE.match(line):
                findings.append(
                    Finding(
                        kind="Figure",
                        file=path.relative_to(repo_root).as_posix(),
                        line=i + 1,
                        detail="Image without #fig caption",
                    )
                )
            continue

        if i + 1 < len(lines) and "|" in line and TABLE_SEP_RE.match(lines[i + 1]):
            j = i - 1
            while j >= 0 and not lines[j].strip():
                j -= 1

            has_caption = j >= 0 and TABLE_CAPTION_RE.match(lines[j])
            if not has_caption:
                findings.append(
                    Finding(
                        kind="Table",
                        file=path.relative_to(repo_root).as_posix(),
                        line=i + 1,
                        detail="Markdown table without Table: #tbl caption",
                    )
                )

    return findings


def write_csv(path: Path, rows: list[Finding]) -> None:
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Type", "File", "Line", "Detail"])
        for r in rows:
            w.writerow([r.kind, r.file, r.line, r.detail])


def main() -> int:
    parser = argparse.ArgumentParser(description="Find figures/tables missing captions in docs markdown files.")
    parser.add_argument("--docs-root", default="docs", help="Docs directory to scan (default: docs)")
    parser.add_argument(
        "--output",
        default="docs_caption_missing_report.csv",
        help="Output CSV report path (default: docs_caption_missing_report.csv)",
    )
    parser.add_argument("--max-print", type=int, default=150, help="Max findings to print (default: 150)")
    args = parser.parse_args()

    repo_root = Path.cwd()
    docs_root = (repo_root / args.docs_root).resolve()
    output = (repo_root / args.output).resolve()

    if not docs_root.exists() or not docs_root.is_dir():
        print(f"Docs root not found: {docs_root}")
        return 2

    findings: list[Finding] = []
    for md in markdown_files(docs_root):
        findings.extend(scan_file(md, repo_root))

    findings.sort(key=lambda r: (r.kind, r.file, r.line))
    write_csv(output, findings)

    fig_count = sum(1 for r in findings if r.kind == "Figure")
    tbl_count = sum(1 for r in findings if r.kind == "Table")

    print(f"Missing captions total: {len(findings)}")
    print(f"Figures missing: {fig_count}")
    print(f"Tables missing: {tbl_count}")
    print(f"Report: {output.relative_to(repo_root).as_posix()}")

    for r in findings[: args.max_print]:
        print(f"{r.kind}|{r.file}|{r.line}|{r.detail}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
