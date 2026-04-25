#!/usr/bin/env python3
"""Audit published C++ parameters against module markdown documentation.

The script scans module source trees for published parameters declared with
PT_* "name", PADDR(...) patterns and checks whether those names appear in the
corresponding module documentation files. It produces reproducible CSV reports
and a JSON summary.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


MODULE_DOC_SUBDIRS: Dict[str, str] = {
    "climate": "Climate",
    "generators": "Generators",
    "powerflow": "Powerflow",
    "residential": "Residential",
    "tape": "Tape",
}

DEFAULT_DOCS_MODULES_ROOT = Path("docs") / "docs" / "3.0 - Modeling Reference" / "Modules"
PARAM_DECL_RE = re.compile(r"PT_[A-Za-z0-9_]+\s*,\s*\"([^\"]+)\"\s*,\s*PADDR")
TYPE_TOKEN_RE = re.compile(r"PT_[A-Za-z0-9_]+")
LINE_COMMENT_RE = re.compile(r"^\s*//")
DESCRIPTION_RE = re.compile(r"PT_DESCRIPTION\s*,\s*\"([^\"]*)\"")

SKIP_TYPE_TOKENS = {
    "PT_KEYWORD",
    "PT_DESCRIPTION",
    "PT_ACCESS",
    "PT_SIZE",
    "PT_UNITS",
    "PT_DEFAULT",
    "PT_REQUIRED",
    "PT_OUTPUT",
    "PT_EXTEND",
}


@dataclass(frozen=True)
class ParameterRecord:
    module: str
    source_file: str
    source_line: int
    parameter: str
    base_parameter: str
    hidden: bool
    description: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root path (default: current directory)",
    )
    parser.add_argument(
        "--docs-modules-root",
        default=str(DEFAULT_DOCS_MODULES_ROOT),
        help=(
            "Path (relative to repo root) to module docs root containing "
            "Climate/Generators/Powerflow/Residential/Tape"
        ),
    )
    parser.add_argument(
        "--output-dir",
        default=str(Path("docs") / "doxygen" / "parameter_audit"),
        help="Path (relative to repo root) for report outputs",
    )
    parser.add_argument(
        "--modules",
        nargs="+",
        default=list(MODULE_DOC_SUBDIRS.keys()),
        help="Module directories to audit (default: climate generators powerflow residential tape)",
    )
    parser.add_argument(
        "--hidden-window-lines",
        type=int,
        default=8,
        help="Number of following lines to inspect for PA_HIDDEN (default: 8)",
    )
    return parser.parse_args()


def normalize_base_parameter(name: str) -> str:
    bracket = name.find("[")
    if bracket > 0 and name.endswith("]"):
        return name[:bracket]
    return name


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def find_parameters_for_module(repo_root: Path, module: str, hidden_window_lines: int) -> List[ParameterRecord]:
    module_root = repo_root / module
    if not module_root.exists():
        return []

    records: List[ParameterRecord] = []
    for source_path in sorted(module_root.rglob("*.cpp")):
        text = read_text(source_path)
        lines = text.splitlines()
        for i, line in enumerate(lines):
            if LINE_COMMENT_RE.match(line):
                continue

            m = PARAM_DECL_RE.search(line)
            if not m:
                continue

            token_match = TYPE_TOKEN_RE.search(line)
            if not token_match:
                continue
            type_token = token_match.group(0)
            if type_token in SKIP_TYPE_TOKENS:
                continue

            param_name = m.group(1)
            base_param = normalize_base_parameter(param_name)
            end = min(i + hidden_window_lines + 1, len(lines))
            window = "\n".join(lines[i:end])
            hidden = "PA_HIDDEN" in window

            # Limit description search to the current parameter tuple segment.
            # This avoids accidentally picking up PT_DESCRIPTION from the next
            # parameter declaration in the same publish call.
            segment_lines = [line[m.end() :]]
            j = i + 1
            while j < len(lines):
                if PARAM_DECL_RE.search(lines[j]):
                    break
                segment_lines.append(lines[j])
                j += 1
            segment = "\n".join(segment_lines)
            desc_match = DESCRIPTION_RE.search(segment)
            description = desc_match.group(1).strip() if desc_match else ""

            records.append(
                ParameterRecord(
                    module=module,
                    source_file=str(source_path.relative_to(repo_root)).replace("\\", "/"),
                    source_line=i + 1,
                    parameter=param_name,
                    base_parameter=base_param,
                    hidden=hidden,
                    description=description,
                )
            )

    unique = sorted(set(records), key=lambda r: (r.module, r.parameter, r.source_file, r.source_line))
    return unique


def list_doc_files(repo_root: Path, docs_modules_root: Path, module: str) -> List[Path]:
    subdir = MODULE_DOC_SUBDIRS.get(module)
    if not subdir:
        return []
    module_doc_root = repo_root / docs_modules_root / subdir
    if not module_doc_root.exists():
        return []
    return sorted(module_doc_root.rglob("*.md"))


def find_match_in_doc_lines(lines_lower: List[str], needle_lower: str) -> Optional[int]:
    for idx, line in enumerate(lines_lower):
        if needle_lower in line:
            return idx + 1
    return None


def find_first_doc_match(
    repo_root: Path,
    doc_files: Iterable[Path],
    parameter: str,
    base_parameter: str,
) -> Tuple[bool, str, int]:
    candidates = []
    for candidate in (parameter.lower(), base_parameter.lower()):
        if candidate and candidate not in candidates:
            candidates.append(candidate)

    for doc_file in doc_files:
        doc_text = read_text(doc_file)
        doc_text_lower = doc_text.lower()
        doc_lines_lower = doc_text_lower.splitlines()

        for needle in candidates:
            if needle not in doc_text_lower:
                continue
            line = find_match_in_doc_lines(doc_lines_lower, needle)
            if line is None:
                line = 1
            rel = str(doc_file.relative_to(repo_root)).replace("\\", "/")
            return True, rel, line

    return False, "", 0


def write_csv(path: Path, rows: List[dict], fieldnames: List[str]) -> None:
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    docs_modules_root = Path(args.docs_modules_root)
    output_dir = (repo_root / Path(args.output_dir)).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    modules = [m.lower() for m in args.modules]
    unknown_modules = [m for m in modules if m not in MODULE_DOC_SUBDIRS]
    if unknown_modules:
        raise SystemExit(f"Unsupported module names: {', '.join(unknown_modules)}")

    all_parameters: List[ParameterRecord] = []
    for module in modules:
        all_parameters.extend(find_parameters_for_module(repo_root, module, args.hidden_window_lines))

    docs_cache: Dict[str, List[Path]] = {
        module: list_doc_files(repo_root, docs_modules_root, module) for module in modules
    }

    all_rows: List[dict] = []
    for rec in all_parameters:
        documented, doc_file, doc_line = find_first_doc_match(
            repo_root=repo_root,
            doc_files=docs_cache.get(rec.module, []),
            parameter=rec.parameter,
            base_parameter=rec.base_parameter,
        )
        all_rows.append(
            {
                "module": rec.module,
                "source_file": rec.source_file,
                "source_line": rec.source_line,
                "parameter": rec.parameter,
                "base_parameter": rec.base_parameter,
                "hidden": rec.hidden,
                "description": rec.description,
                "documented": documented,
                "doc_file": doc_file,
                "doc_line": doc_line,
            }
        )

    all_rows.sort(key=lambda r: (r["module"], r["parameter"], r["source_file"], r["source_line"]))

    public_missing = [r for r in all_rows if (not r["hidden"] and not r["documented"])]
    hidden_in_docs = [r for r in all_rows if (r["hidden"] and r["documented"])]

    write_csv(
        output_dir / "all_parameters.csv",
        all_rows,
        [
            "module",
            "source_file",
            "source_line",
            "parameter",
            "base_parameter",
            "hidden",
            "description",
            "documented",
            "doc_file",
            "doc_line",
        ],
    )
    write_csv(
        output_dir / "public_missing_docs.csv",
        public_missing,
        [
            "module",
            "source_file",
            "source_line",
            "parameter",
            "base_parameter",
            "hidden",
            "description",
            "documented",
            "doc_file",
            "doc_line",
        ],
    )
    write_csv(
        output_dir / "hidden_found_in_docs.csv",
        hidden_in_docs,
        [
            "module",
            "source_file",
            "source_line",
            "parameter",
            "base_parameter",
            "hidden",
            "description",
            "documented",
            "doc_file",
            "doc_line",
        ],
    )

    summary = {
        "repo_root": str(repo_root),
        "docs_modules_root": str((repo_root / docs_modules_root).resolve()),
        "modules": modules,
        "total_parameters": len(all_rows),
        "hidden_parameters": sum(1 for r in all_rows if r["hidden"]),
        "public_parameters": sum(1 for r in all_rows if not r["hidden"]),
        "public_missing_docs": len(public_missing),
        "hidden_found_in_docs": len(hidden_in_docs),
        "public_missing_by_module": {
            m: sum(1 for r in public_missing if r["module"] == m) for m in modules
        },
        "hidden_found_by_module": {
            m: sum(1 for r in hidden_in_docs if r["module"] == m) for m in modules
        },
        "reports": {
            "all_parameters_csv": str((output_dir / "all_parameters.csv").relative_to(repo_root)).replace("\\", "/"),
            "public_missing_docs_csv": str((output_dir / "public_missing_docs.csv").relative_to(repo_root)).replace("\\", "/"),
            "hidden_found_in_docs_csv": str((output_dir / "hidden_found_in_docs.csv").relative_to(repo_root)).replace("\\", "/"),
            "summary_json": str((output_dir / "summary.json").relative_to(repo_root)).replace("\\", "/"),
        },
    }

    summary_path = output_dir / "summary.json"
    summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print("Parameter documentation audit complete")
    print(f"Total parameters: {summary['total_parameters']}")
    print(f"Public missing docs: {summary['public_missing_docs']}")
    print(f"Hidden found in docs: {summary['hidden_found_in_docs']}")
    print(f"Output directory: {str(output_dir.relative_to(repo_root)).replace('\\', '/')}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
