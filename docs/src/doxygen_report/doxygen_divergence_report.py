#!/usr/bin/env python3
"""Generate a markdown report from Doxygen warnings.

This script reads a Doxygen warning log and creates a grouped report showing
where source code and API documentation diverge.
"""

from __future__ import annotations

import argparse
import os
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, List, Tuple

WARNING_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):\s*warning:\s*(?P<msg>.+)$")
WARNING_NO_LINE_RE = re.compile(r"^(?P<file>.+?):\s*warning:\s*(?P<msg>.+)$")


def classify_warning(message: str) -> str:
    text = message.lower()
    if "not documented" in text:
        return "Undocumented API"
    if "argument" in text and "not found" in text:
        return "Parameter Mismatch"
    if "unable to resolve reference" in text or "not found" in text:
        return "Broken Reference"
    if "syntax" in text or "parse" in text:
        return "Doc Syntax Issue"
    return "Other"


def parse_warnings(log_path: Path) -> List[Dict[str, str]]:
    items: List[Dict[str, str]] = []
    if not log_path.exists():
        return items

    with log_path.open("r", encoding="utf-8", errors="replace") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue

            match = WARNING_RE.match(line)
            if match:
                file_path = match.group("file")
                line_number = match.group("line")
                message = match.group("msg")
            else:
                match_no_line = WARNING_NO_LINE_RE.match(line)
                if not match_no_line:
                    continue
                file_path = match_no_line.group("file")
                line_number = "-"
                message = match_no_line.group("msg")

            items.append(
                {
                    "file": file_path,
                    "line": line_number,
                    "message": message,
                    "category": classify_warning(message),
                }
            )

    return items


def group_by_category(items: List[Dict[str, str]]) -> Dict[str, List[Dict[str, str]]]:
    grouped: Dict[str, List[Dict[str, str]]] = defaultdict(list)
    for item in items:
        grouped[item["category"]].append(item)
    return grouped


def to_relative(path_text: str, repo_root: Path) -> str:
    path = Path(path_text)
    if not path.is_absolute():
        return path_text.replace("\\", "/")

    try:
        return str(path.resolve().relative_to(repo_root.resolve())).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


def write_report(
    output_path: Path,
    warnings: List[Dict[str, str]],
    repo_root: Path,
    source_log: Path,
) -> None:
    grouped = group_by_category(warnings)
    file_counts: Counter[str] = Counter(to_relative(item["file"], repo_root) for item in warnings)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as handle:
        handle.write("# Doxygen Documentation Divergence Report\n\n")
        handle.write("This report is generated from Doxygen warnings emitted during the documentation build.\n\n")
        handle.write(f"Source log: {source_log.as_posix()}\n\n")

        handle.write("## Summary\n\n")
        handle.write(f"- Total warnings: {len(warnings)}\n")
        handle.write(f"- Files with warnings: {len(file_counts)}\n")

        if grouped:
            for category, entries in sorted(grouped.items()):
                handle.write(f"- {category}: {len(entries)}\n")
        handle.write("\n")

        handle.write("## Most Affected Files\n\n")
        if file_counts:
            for file_path, count in file_counts.most_common(25):
                handle.write(f"- {file_path}: {count}\n")
        else:
            handle.write("- No warnings found\n")
        handle.write("\n")

        handle.write("## Detailed Findings\n\n")
        if not warnings:
            handle.write("No documentation divergence warnings were found.\n")
            return

        for category in sorted(grouped.keys()):
            handle.write(f"### {category}\n\n")
            for item in grouped[category]:
                rel_file = to_relative(item["file"], repo_root)
                handle.write(
                    f"- {rel_file}:{item['line']} - {item['message']}\n"
                )
            handle.write("\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a markdown source/documentation divergence report from Doxygen warnings."
    )
    parser.add_argument(
        "--log",
        default="docs/doxygen-warnings.log",
        help="Path to Doxygen warning log file.",
    )
    parser.add_argument(
        "--output",
        default="docs/doxygen_divergence_report.md",
        help="Path to output markdown report.",
    )
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root used to shorten absolute paths.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    log_path = (repo_root / args.log).resolve() if not Path(args.log).is_absolute() else Path(args.log)
    output_path = (repo_root / args.output).resolve() if not Path(args.output).is_absolute() else Path(args.output)

    warnings = parse_warnings(log_path)
    write_report(output_path, warnings, repo_root, log_path)

    print(f"Parsed warnings: {len(warnings)}")
    print(f"Report written: {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
