#!/usr/bin/env python3
"""Generate powerflow class docs from C++ source.

Features:
- Single-class generation from source_path
- Batch generation for all discovered powerflow classes
- Post-processing with existing docs to reuse descriptions
"""

from __future__ import annotations

import argparse
import html
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Sequence

# Constants
ACCESS_DEFAULT = "PA_PUBLIC"
DEVELOPER_ACCESS = "PA_HIDDEN"
NON_INPUT_ACCESS = {"PA_REFERENCE", "PA_PROTECTED", "PA_PRIVATE", "PA_HIDDEN"}
INIT_METHODS = {"create", "init"}
RUNTIME_METHODS = {"presync", "sync", "postsync", "commit", "precommit", "update", "interupdate", "notify"}
TABLE_HEADERS = {"property name", "parameter", "property"}
LEGACY_CLASS_NAME_MAPPING = {
    "link_object": "link", "switch_object": "switch", "triplex_line_conductor": "triplex_conductor",
    "pqload": "pqload", "currdump": "current_dump", "billdump": "bill_dump", "voltdump": "volt_dump",
}


@dataclass
class PropertyRow:
    """Property row in documentation table."""
    name: str
    ptype: str
    unit: str
    description: str
    enum_keywords: List[str]
    addr_expr: str
    access: str
    is_input: bool = False
    has_runtime_updates: bool = False
    evidence: str = ""
    description_from_existing: bool = False


@dataclass
class PropertySection:
    """Section of properties."""
    heading: str
    intro: str
    rows: List[PropertyRow]


@dataclass
class ExistingDocContent:
    """Content extracted from existing documentation."""
    intro_lines: List[str]
    extra_sections: List[List[str]]
    descriptions_by_property: dict[str, str]


def title_from_class(class_name: str) -> str:
    if class_name.lower() == "vfd": return "VFD"
    return class_name.replace("_", " ").title()


def parse_property_name_and_unit(raw_name: str) -> tuple[str, str]:
    match = re.match(r"^(?P<name>.+)\[(?P<unit>[^\]]+)\]$", raw_name)
    return (match.group("name"), match.group("unit")) if match else (raw_name, "N/A")


def normalize_type(pt: str) -> str:
    return pt.lower()


def extract_addr_expr(line: str) -> str:
    match = re.search(r"PADDR\((?P<addr>.*?)\)\s*,", line)
    return match.group("addr").strip() if match else ""


def extract_base_symbol(addr: str) -> str:
    match = re.match(r"(?P<base>[A-Za-z_][A-Za-z0-9_]*)", addr)
    return match.group("base") if match else ""


def access_from_line(line: str) -> str | None:
    match = re.search(r"PT_ACCESS\s*,\s*(PA_[A-Z_]+)", line)
    return match.group(1) if match else None


def format_description(row: PropertyRow) -> str:
    """Format description with enum keywords and source warning emoji."""
    desc = row.description.strip()
    if row.enum_keywords:
        norm_desc = " ".join(desc.lower().split())
        has_valid = bool(re.search(r"\bvalid\b.*\b(values?|keywords?)\b", norm_desc))
        has_tokens = any(f"`{v}`" in desc for v in row.enum_keywords)
        if not has_valid and not has_tokens:
            enum_text = ", ".join(f"`{v}`" for v in row.enum_keywords)
            desc = f"{desc} Valid values: {enum_text}." if desc else f"Valid values: {enum_text}."
    return f"⚠️ {desc}" if not row.description_from_existing and desc else desc


def io_label(row: PropertyRow) -> str:
    """Return merged I/O marker for documentation tables."""
    if row.is_input and row.has_runtime_updates:
        return "IO"
    if row.is_input:
        return "I"
    if row.has_runtime_updates:
        return "O"
    return "—"


def classify_method(name: str, class_name: str) -> str:
    """Classify method: 'init' (constructors), 'runtime' (sync), 'other'."""
    low = name.lower()
    cls_low = class_name.lower()
    if low in {cls_low, f"~{cls_low}"} or low in INIT_METHODS: return "init"
    if any(t in low for t in RUNTIME_METHODS): return "runtime"
    return "other"


def find_runtime_writes(src: str, cls_name: str, bases: Sequence[str]) -> dict[str, dict[str, set[str]]]:
    """Scan source to find write locations (init/runtime/other) for each symbol."""
    writes = {b: {"init": set(), "runtime": set(), "other": set()} for b in bases if b}
    if not writes: return writes
    
    method_pat = re.compile(rf"\b{re.escape(cls_name)}::(?P<method>~?\w+)\s*\(")
    curr_method, curr_bucket = "<global>", "other"
    
    for line in src.splitlines():
        m = method_pat.search(line)
        if m: curr_method, curr_bucket = m.group("method"), classify_method(m.group("method"), cls_name)
        for base in writes:
            if re.search(rf"(?<!\.)(?<!->)\b{re.escape(base)}\b(?:\s*(?:\[[^\]]+\]|\.[A-Za-z_][A-Za-z0-9_]*(?:\(\))?))*\s*(?:[+\-*/%&|^]?=(?!=)|\+\+|--)", line) or \
               re.search(rf"(?<!\.)(?<!->)\b{re.escape(base)}\b(?:\s*(?:\[[^\]]+\]|\.[A-Za-z_][A-Za-z0-9_]*(?:\(\))?))*\.Set[A-Za-z0-9_]*\s*\(", line):
                writes[base][curr_bucket].add(curr_method)
    return writes


def classify_property(row: PropertyRow, writes: dict[str, dict[str, set[str]]]) -> None:
    """Classify property as input and/or runtime-modified."""
    base = extract_base_symbol(row.addr_expr)
    w = writes.get(base, {"init": set(), "runtime": set(), "other": set()})
    has_non_init_writes = bool(w["runtime"] or w["other"])
    row.has_runtime_updates = has_non_init_writes
    row.is_input = False if (row.access in NON_INPUT_ACCESS or (has_non_init_writes and not w["init"])) else True
    
    parts = []
    if row.access != ACCESS_DEFAULT: parts.append(f"access={row.access}")
    if w["init"]: parts.append("init: " + ", ".join(sorted(w["init"])))
    if w["runtime"]: parts.append("runtime: " + ", ".join(sorted(w["runtime"])))
    if w["other"]: parts.append("other: " + ", ".join(sorted(w["other"])))
    row.evidence = "; ".join(parts) if parts else "default PA_PUBLIC; no writes detected"


def extract_from_source(cls: str, src: str) -> tuple[List[str], List[PropertySection], List[PropertyRow]]:
    """Extract class definition, inheritance, and properties from C++ source."""
    parents, props = [], []
    last_prop, in_block = None, False
    
    for line in src.splitlines():
        if "gl_publish_variable(" in line: in_block = True
        if not in_block: continue
        if "PT_" not in line or '"' not in line:
            if "nullptr" in line: break
            continue
        
        m = re.search(r'PT_(?P<t>[A-Za-z0-9_]+)\s*,\s*"(?P<v>[^"]+)"', line)
        if not m:
            if "nullptr" in line: break
            continue
        
        t, v = m.group("t"), m.group("v")
        if t == "KEYWORD":
            if last_prop: last_prop.enum_keywords.append(v)
            continue
        if t == "ACCESS":
            if last_prop: last_prop.access = v
            continue
        if t == "INHERIT":
            parents.append(v)
            continue
        
        desc_m = re.search(r'PT_DESCRIPTION\s*,\s*"(?P<d>[^"]*)"', line)
        name, unit = parse_property_name_and_unit(v)
        row = PropertyRow(name, normalize_type(t), unit, desc_m.group("d") if desc_m else "", [], 
                         extract_addr_expr(line), access_from_line(line) or ACCESS_DEFAULT)
        props.append(row)
        last_prop = row
        if "nullptr" in line: break
    
    # Deduplicate
    def dedup(items): 
        seen, res = set(), []
        for item in items:
            k = (item.name if isinstance(item, PropertyRow) else item)
            if k not in seen: seen.add(k); res.append(item)
        return res
    
    props, parents = dedup(props), dedup(parents)
    sections = [PropertySection("Properties", "", props)] if props else []
    
    writes = find_runtime_writes(src, cls, [extract_base_symbol(p.addr_expr) for p in props])
    for p in props: classify_property(p, writes)
    
    return parents, sections, props


def parse_table_cells(line: str) -> List[str]:
    if "|" not in line: return []
    c = [x.strip() for x in line.split("|")]
    if c and c[0] == "": c = c[1:]
    if c and c[-1] == "": c = c[:-1]
    return c


def is_separator_row(cells: Sequence[str]) -> bool:
    return bool(cells) and all(x and set(x) <= {'-', ' ', ':'} for x in cells)


def extract_existing_descriptions(doc: str) -> dict[str, str]:
    """Extract property descriptions from existing markdown."""
    descs = {}
    lines = doc.splitlines()
    i = 0
    
    while i < len(lines):
        h = parse_table_cells(lines[i])
        if not h: i += 1; continue
        lh = [x.lower() for x in h]
        if not lh or lh[0] not in TABLE_HEADERS or "description" not in lh: i += 1; continue
        
        d_idx = lh.index("description")
        j = i + 1
        if j < len(lines):
            s = parse_table_cells(lines[j])
            if s and is_separator_row(s): j += 1
        
        while j < len(lines):
            if re.match(r"^#{2,6}\s+", lines[j].strip()): break
            r = parse_table_cells(lines[j])
            if not r: j += 1; continue
            if is_separator_row(r): j += 1; continue
            if len(r) > max(d_idx, 0):
                name = r[0].replace("**", "").strip()
                desc = r[d_idx].strip() if d_idx < len(r) else ""
                if name and desc and name not in descs: descs[name] = desc
            j += 1
        i = j
    
    return descs


def extract_existing_intro_and_sections(doc: str) -> tuple[List[str], List[List[str]]]:
    """Extract intro and extra sections from existing documentation."""
    lines = doc.splitlines()
    
    # Find main heading
    title_i = next((i for i, l in enumerate(lines) if l.strip().startswith("## ")), -1)
    intro = []
    if title_i >= 0:
        h3_i = next((i for i in range(title_i + 1, len(lines)) if lines[i].strip().startswith("### ")), len(lines))
        intro = lines[title_i + 1:h3_i]
        while intro and not intro[0].strip(): intro = intro[1:]
        while intro and not intro[-1].strip(): intro = intro[:-1]
    
    # Find extra sections
    extra = []
    h3s = [i for i, l in enumerate(lines) if l.strip().startswith("### ")]
    for j, start in enumerate(h3s):
        end = h3s[j + 1] if j + 1 < len(h3s) else len(lines)
        head = lines[start].strip().lower()
        if any(k in head for k in ["inheritance", "parents", "parameters"]): continue
        b = lines[start:end]
        while b and not b[-1].strip(): b = b[:-1]
        if b: extra.append(b)
    
    # Related concepts
    rel_i = next((i for i, l in enumerate(lines) if re.match(r"^#{1,3}\s*Related Concepts", l.strip(), re.I)), -1)
    if rel_i >= 0:
        rel = lines[rel_i:]
        while rel and not rel[-1].strip(): rel = rel[:-1]
        if rel: extra.append(rel)
    
    return intro, extra


def extract_existing_doc_content(doc: str) -> ExistingDocContent:
    intro, extra = extract_existing_intro_and_sections(doc)
    descs = extract_existing_descriptions(doc)
    return ExistingDocContent(intro, extra, descs)


def apply_existing_descriptions(rows: Sequence[PropertyRow], descs: dict[str, str]) -> None:
    for row in rows:
        if row.name in descs:
            row.description = descs[row.name]
            row.description_from_existing = True


def markdown_parent_ref(cls: str, docs_dir: Path) -> str:
    """Generate markdown link to parent class doc."""
    doc = _find_existing_doc(cls, docs_dir)
    name = _output_filename(cls, doc)
    return f"**[{cls}]({name})**"


def _find_existing_doc(cls: str, docs_dir: Path) -> Path | None:
    for n in [cls, LEGACY_CLASS_NAME_MAPPING.get(cls, cls)]:
        for p in [f"* - {n}.md", f"*-{n}.md"]:
            m = sorted(docs_dir.glob(p))
            if m: return m[0]
    return None


def _output_filename(cls: str, doc: Path | None) -> str:
    n = doc.name if doc else f"{LEGACY_CLASS_NAME_MAPPING.get(cls, cls)}.md"
    return n.replace(" ", "")


def markdown_for_class(cls: str, parents: Sequence[str], sections: Sequence[PropertySection], 
                      docs_dir: Path, intro: Sequence[str] | None = None, 
                      extra: Sequence[Sequence[str]] | None = None) -> str:
    """Generate complete markdown documentation for a class."""
    out = [f"## {title_from_class(cls)}", "", "!!! warning", "    This page was automatically generated and requires review.", ""]
    
    if intro: out.extend(intro); out.append("")
    
    # Inheritance prose
    if parents:
        if len(parents) == 1:
            pref = markdown_parent_ref(parents[0], docs_dir)
            inh = [f"**{cls}** objects are derived from {pref} objects, so any parameters of the {pref} object are available as well."]
        else:
            prefs = ", ".join(markdown_parent_ref(p, docs_dir) for p in parents)
            inh = [f"**{cls}** objects are derived from {prefs} objects, so any parameters of those parent objects are available as well."]
    else:
        inh = [f"**{cls}** does not declare inherited parent classes."]
    
    out.append(f"### {title_from_class(cls)} Parameters")
    out.append("")
    
    for sec in sections:
        user = [r for r in sec.rows if r.access != DEVELOPER_ACCESS]
        dev = [r for r in sec.rows if r.access == DEVELOPER_ACCESS]
        
        if user:
            out.append(f"#### {sec.heading}")
            out.append("")
            out.extend(inh)
            out.append("")
            out.append("The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).")
            out.append("")
            out.append("| Property Name | Type | Unit | I/O | Description |")
            out.append("| --- | --- | --- | --- | --- |")
            for r in user:
                desc = format_description(r)
                if not r.description_from_existing:
                    desc = html.escape(desc)
                d = desc.replace("|", "&#124;")
                # d = html.escape(format_description(r)).replace("|", "&#124;")
                out.append(f"| {r.name} | {r.ptype} | {r.unit} | {io_label(r)} | {d} |")
            out.append("")
        
        if dev:
            out.append("#### Developer Properties")
            out.append("")
            if not user: out.extend(inh); out.append("")
            out.append("These properties are published with `PA_HIDDEN` and are intended for internal or developer use.")
            out.append("")
            out.append("| Property Name | Type | Unit | I/O | Description |")
            out.append("| --- | --- | --- | --- | --- |")
            for r in dev:
                desc = format_description(r)
                if not r.description_from_existing:
                    desc = html.escape(desc)
                d = desc.replace("|", "&#124;")
                # d = html.escape(format_description(r)).replace("|", "&#124;")
                out.append(f"| {r.name} | {r.ptype} | {r.unit} | {io_label(r)} | {d} |")
            out.append("")
    
    if extra:
        for b in extra: out.extend(b); out.append("")
    
    return "\n".join(out)


def extract_doc_properties(doc: str) -> List[str]:
    """Extract property names from doc tables."""
    names = []
    for line in doc.splitlines():
        c = parse_table_cells(line.strip())
        if c and len(c) >= 4 and not is_separator_row(c) and c[0].lower() not in TABLE_HEADERS:
            n = c[0].replace("**", "").strip()
            if n: names.append(n)
    
    seen, res = set(), []
    for n in names:
        if n not in seen: seen.add(n); res.append(n)
    return res


def write_comparison_report(gen: Iterable[PropertyRow], doc: Sequence[str], cls: str, path: Path) -> None:
    """Generate comparison report between source-derived and legacy properties."""
    gen_set = {r.name for r in gen}
    doc_set = set(doc)
    src_only = sorted(gen_set - doc_set)
    doc_only = sorted(doc_set - gen_set)
    
    lines = [f"# Comparison Report: {title_from_class(cls)}", "", 
             f"- Source properties: {len(gen_set)}",
             f"- Doc properties: {len(doc_set)}",
             f"- In source only: {len(src_only)}",
             f"- In doc only: {len(doc_only)}", ""]
    
    lines.extend(["## In Source Only", "", "| Property |", "| --- |"] + 
                 [f"| {n} |" for n in src_only] + 
                 ["", "## In Doc Only", "", "| Property |", "| --- |"] +
                 [f"| {n} |" for n in doc_only] + [""])
    
    path.write_text("\n".join(lines), encoding="utf-8")


def discover_classes(pf_dir: Path) -> List[tuple[str, Path]]:
    """Discover all powerflow classes in C++ source files."""
    pat = re.compile(r"CLASS\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)::oclass")
    res = []
    for f in sorted(pf_dir.glob("*.cpp")):
        m = pat.search(f.read_text(encoding="utf-8", errors="ignore"))
        if m: res.append((m.group(1), f))
    return res


def generate_one(cls: str, src: Path, out: Path, exists: Path | None, docs_dir: Path, 
                cmp_doc: Path | None, cmp_out: Path | None, no_pp: bool) -> tuple[int, int, int]:
    """Generate markdown documentation for a single class."""
    src_text = src.read_text(encoding="utf-8", errors="ignore")
    parents, secs, props = extract_from_source(cls, src_text)
    
    intro, extra = [], []
    if exists and exists.exists() and not no_pp:
        c = extract_existing_doc_content(exists.read_text(encoding="utf-8", errors="ignore"))
        apply_existing_descriptions(props, c.descriptions_by_property)
        intro, extra = c.intro_lines, c.extra_sections
    
    md = markdown_for_class(cls, parents, secs, docs_dir, intro, extra)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(md, encoding="utf-8")
    
    if cmp_doc and cmp_out:
        write_comparison_report(props, extract_doc_properties(cmp_doc.read_text(encoding="utf-8", errors="ignore")), cls, cmp_out)
    
    return len(parents), len(secs), len(props)


def generate_all(args) -> None:
    """Generate markdown for all discovered powerflow classes."""
    pf, out_d, ex_d = Path(args.powerflow_dir), Path(args.output_dir), Path(args.existing_docs_dir)
    clses = discover_classes(pf)
    if not clses: raise ValueError(f"No classes found in {pf}")
    
    for cnt, (cls, src_p) in enumerate(clses, 1):
        ex = _find_existing_doc(cls, ex_d)
        out_p = out_d / _output_filename(cls, ex)
        i, s, p = generate_one(cls, src_p, out_p, ex, ex_d, None, None, args.no_postprocess_existing)
        print(f"Generated: {out_p} ({i} parents, {s} sections, {p} properties)")
        
        for st in {f"{cls}.md", f"{LEGACY_CLASS_NAME_MAPPING.get(cls, cls)}.md", ex.name if ex else None}:
            if st and (sp := out_d / st) != out_p and sp.exists(): sp.unlink()
    
    print(f"Generated {cnt} files in {out_d}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate powerflow class docs from C++ source.")
    parser.add_argument("--class-name", default=None, help="Class name (single-class mode)")
    parser.add_argument("--source", default=None, help="Source file (single-class mode)")
    parser.add_argument("--output", default=None, help="Output file (single-class mode)")
    parser.add_argument("--all-powerflow", action="store_true", help="Generate all classes")
    parser.add_argument("--powerflow-dir", default="powerflow", help="Powerflow source directory")
    parser.add_argument("--output-dir", default="docs/docs/3.0 - Modeling Reference/Modules/Powerflow/Classes_generated", help="Output directory")
    parser.add_argument("--existing-docs-dir", default="docs/docs/3.0 - Modeling Reference/Modules/Powerflow/Classes_original", help="Legacy docs directory")
    parser.add_argument("--existing-doc", default=None, help="Existing doc for post-processing")
    parser.add_argument("--no-postprocess-existing", action="store_true", help="Skip post-processing")
    parser.add_argument("--compare-doc", default=None, help="Doc for comparison report")
    parser.add_argument("--compare-output", default=None, help="Comparison report output")
    
    args = parser.parse_args()
    
    if args.all_powerflow:
        generate_all(args)
        return
    
    if not args.class_name or not args.source or not args.output:
        raise ValueError("Single-class needs --class-name, --source, --output")
    
    ex = Path(args.existing_doc) if args.existing_doc else (_find_existing_doc(args.class_name, Path(args.existing_docs_dir)) if not args.no_postprocess_existing else None)
    i, s, p = generate_one(args.class_name, Path(args.source), Path(args.output), ex, Path(args.existing_docs_dir), 
                          Path(args.compare_doc) if args.compare_doc else None, 
                          Path(args.compare_output) if args.compare_output else None, args.no_postprocess_existing)
    print(f"Generated: {args.output} ({i} parents, {s} sections, {p} properties)")
    if args.compare_doc and args.compare_output: print(f"Comparison report: {args.compare_output}")


if __name__ == "__main__":
    main()
