"""GridLAB-D model editor GUI.

This script provides a GUI for editing groups of object properties in a GLM.
Each property rule has a class filter and applies uniform random edits only to
objects in the selected class scope.
"""

from __future__ import annotations

import random
import re
import tkinter as tk
from dataclasses import dataclass
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import gridlabd


CLASS_FILTER_CHOICES = ["all", "house", "solar", "inverter"]

CLASS_FILTER_MAP = {
	"all": ["house", "house_e", "solar", "inverter"],
	"house": ["house", "house_e"],
	"solar": ["solar"],
	"inverter": ["inverter"],
}

MODEL_SPECS = {
	"house": {
		"header": Path("residential/house_e.h"),
		"source": Path("residential/house_e.cpp"),
	},
	"solar": {
		"header": Path("generators/solar.h"),
		"source": Path("generators/solar.cpp"),
	},
	"inverter": {
		"header": Path("generators/inverter.h"),
		"source": Path("generators/inverter.cpp"),
	},
}

PUBLISHABLE_TYPES = {
	"PT_double",
	"PT_int16",
	"PT_int32",
	"PT_int64",
	"PT_bool",
	"PT_complex",
	"PT_object",
	"PT_enumeration",
	"PT_set",
	"PT_timestamp",
	"PT_char1024",
	"PT_char256",
	"PT_char32",
	"PT_char8",
	"PT_random",
	"PT_method",
}


def _extract_numeric(value) -> float | None:
	"""Extract a numeric value from GridLAB-D property strings."""
	if isinstance(value, (int, float)):
		return float(value)
	if value is None:
		return None

	text = str(value).strip()
	match = re.search(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", text)
	if not match:
		return None
	try:
		return float(match.group(0))
	except ValueError:
		return None


def _strip_comments(text: str) -> str:
	text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
	text = re.sub(r"//.*", "", text)
	return text


def _parse_header_properties(header_path: Path) -> set[str]:
	"""Parse public member names from a class header.

	This keeps parsing lightweight and focuses on public declarations.
	"""
	if not header_path.exists():
		return set()

	text = _strip_comments(header_path.read_text(encoding="utf-8", errors="ignore"))
	props: set[str] = set()
	access = "private"

	for raw in text.splitlines():
		line = raw.strip()
		if not line:
			continue
		if line.endswith(":") and line[:-1] in {"public", "private", "protected"}:
			access = line[:-1]
			continue
		if access != "public":
			continue
		if ";" not in line:
			continue
		if any(tok in line for tok in ("(", ")", "typedef", "enum", "class ", "#", "{", "}")):
			continue

		decl = line.split(";", 1)[0]
		decl = decl.split("=", 1)[0]
		chunks = [x.strip() for x in decl.split(",") if x.strip()]
		if not chunks:
			continue

		for idx, chunk in enumerate(chunks):
			m = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?$", chunk)
			if not m:
				continue
			name = m.group(1)
			if idx == 0 and name in {"double", "bool", "int", "enumeration", "complex", "TIMESTAMP"}:
				continue
			props.add(name)

	return props


def _parse_published_properties_and_units(source_path: Path) -> tuple[set[str], dict[str, str]]:
	"""Parse gl_publish_variable-style property names and units from a .cpp file."""
	if not source_path.exists():
		return set(), {}

	text = source_path.read_text(encoding="utf-8", errors="ignore")
	props: set[str] = set()
	units: dict[str, str] = {}

	for m in re.finditer(r"\b(PT_[A-Za-z0-9_]+)\s*,\s*\"([^\"]+)\"", text):
		ptype = m.group(1)
		raw_name = m.group(2).strip()
		if ptype not in PUBLISHABLE_TYPES:
			continue

		name_match = re.match(r"^([^\[]+?)(?:\[([^\]]+)\])?$", raw_name)
		if not name_match:
			continue
		name = name_match.group(1).strip()
		unit = (name_match.group(2) or "").strip()
		if not name:
			continue

		props.add(name)
		if unit:
			units[name] = unit

	return props, units


def _find_repo_root(start: Path) -> Path | None:
	for candidate in [start] + list(start.parents):
		if (candidate / "residential/house_e.h").exists() and (candidate / "generators/solar.h").exists():
			return candidate
	return None


@dataclass
class PropertyRule:
	class_filter: str
	property_name: str
	unit: str
	min_value: float
	max_value: float
	show_histogram: bool


class PropertyRuleRow:
	"""One dynamic row in the property rule list."""

	def __init__(self, parent: tk.Widget, on_remove, metadata_provider):
		self.frame = ttk.Frame(parent)
		self.on_remove = on_remove
		self.metadata_provider = metadata_provider

		self.class_var = tk.StringVar(value="all")
		self.property_var = tk.StringVar()
		self.unit_var = tk.StringVar(value="-")
		self.min_var = tk.StringVar()
		self.max_var = tk.StringVar()
		self.hist_var = tk.BooleanVar(value=True)

		self.class_combo = ttk.Combobox(
			self.frame,
			textvariable=self.class_var,
			values=CLASS_FILTER_CHOICES,
			state="readonly",
			width=14,
		)
		self.class_combo.grid(row=0, column=0, padx=4, pady=4, sticky="ew")

		self.property_combo = ttk.Combobox(
			self.frame,
			textvariable=self.property_var,
			values=[],
			state="normal",
			width=32,
		)
		self.property_combo.grid(row=0, column=1, padx=4, pady=4, sticky="ew")

		ttk.Entry(self.frame, textvariable=self.min_var, width=12).grid(
			row=0, column=2, padx=4, pady=4, sticky="ew"
		)
		ttk.Entry(self.frame, textvariable=self.max_var, width=12).grid(
			row=0, column=3, padx=4, pady=4, sticky="ew"
		)

		ttk.Label(self.frame, textvariable=self.unit_var, width=16).grid(
			row=0, column=4, padx=4, pady=4, sticky="w"
		)

		ttk.Checkbutton(self.frame, variable=self.hist_var).grid(
			row=0, column=5, padx=4, pady=4
		)
		ttk.Button(self.frame, text="Remove", command=self.remove).grid(
			row=0, column=6, padx=4, pady=4
		)

		self.class_combo.bind("<<ComboboxSelected>>", self._on_class_changed)
		self.property_combo.bind("<<ComboboxSelected>>", self._on_property_changed)
		self.property_combo.bind("<KeyRelease>", self._on_property_typed)

		self._refresh_property_values()

	def _property_choices(self) -> list[str]:
		return self.metadata_provider("choices", self.class_var.get().strip(), "")

	def _property_unit(self, name: str) -> str:
		return self.metadata_provider("unit", self.class_var.get().strip(), name)

	def _refresh_property_values(self) -> None:
		choices = self._property_choices()
		self.property_combo["values"] = choices
		current = self.property_var.get().strip()
		if current and current not in choices:
			self.unit_var.set("-")
		else:
			self.unit_var.set(self._property_unit(current) or "-")

	def _on_class_changed(self, _event=None) -> None:
		self._refresh_property_values()

	def _on_property_changed(self, _event=None) -> None:
		name = self.property_var.get().strip()
		self.unit_var.set(self._property_unit(name) or "-")

	def _on_property_typed(self, _event=None) -> None:
		typed = self.property_var.get().strip().lower()
		choices = self._property_choices()
		if typed:
			filtered = [x for x in choices if typed in x.lower()]
			self.property_combo["values"] = filtered
		else:
			self.property_combo["values"] = choices
		self.unit_var.set(self._property_unit(self.property_var.get().strip()) or "-")

	def remove(self) -> None:
		self.frame.destroy()
		self.on_remove(self)

	def get_rule(self) -> PropertyRule:
		class_filter = self.class_var.get().strip()
		if class_filter not in CLASS_FILTER_MAP:
			raise ValueError(f"Invalid class filter '{class_filter}'.")

		name = self.property_var.get().strip()
		if not name:
			raise ValueError("Property name cannot be empty.")

		if class_filter != "all":
			valid = self.metadata_provider("choices", class_filter, "")
			if valid and name not in valid:
				raise ValueError(
					f"Property '{name}' is not a recognized property for class '{class_filter}'.\n"
					f"Check the property name or change the class filter."
				)

		try:
			min_val = float(self.min_var.get().strip())
			max_val = float(self.max_var.get().strip())
		except ValueError as exc:
			raise ValueError(f"Property '{name}' min/max must be numeric.") from exc

		if min_val > max_val:
			raise ValueError(f"Property '{name}' has min greater than max.")

		return PropertyRule(
			class_filter=class_filter,
			property_name=name,
			unit=self._property_unit(name),
			min_value=min_val,
			max_value=max_val,
			show_histogram=self.hist_var.get(),
		)


class ModelEditGUI:
	def __init__(self, root: tk.Tk):
		self.root = root
		self.root.title("GridLAB-D Property Group Editor")
		self.root.geometry("1200x800")

		self.input_file_var = tk.StringVar()
		self.output_file_var = tk.StringVar()
		self.seed_var = tk.StringVar()
		self.status_var = tk.StringVar(value="Select an input GLM and define property rules.")

		self.class_properties, self.class_units = self._load_property_metadata()
		self.rule_rows: list[PropertyRuleRow] = []

		self._build_ui()
		self._add_rule_row()

	def _load_property_metadata(self) -> tuple[dict[str, list[str]], dict[str, dict[str, str]]]:
		class_properties: dict[str, list[str]] = {"house": [], "solar": [], "inverter": []}
		class_units: dict[str, dict[str, str]] = {"house": {}, "solar": {}, "inverter": {}}

		repo_root = _find_repo_root(Path(__file__).resolve().parent)
		if repo_root is None:
			return class_properties, class_units

		for cls_name, spec in MODEL_SPECS.items():
			header_props = _parse_header_properties(repo_root / spec["header"])
			published_props, published_units = _parse_published_properties_and_units(repo_root / spec["source"])
			combined = sorted(header_props | published_props)
			class_properties[cls_name] = combined
			class_units[cls_name] = published_units

		return class_properties, class_units

	def _metadata_provider(self, mode: str, class_filter: str, prop_name: str):
		if class_filter not in CLASS_FILTER_MAP:
			return [] if mode == "choices" else ""

		display_classes: list[str]
		if class_filter == "all":
			display_classes = ["house", "solar", "inverter"]
		else:
			display_classes = [class_filter]

		if mode == "choices":
			choices: set[str] = set()
			for cls_name in display_classes:
				choices.update(self.class_properties.get(cls_name, []))
			return sorted(choices)

		if mode == "unit":
			for cls_name in display_classes:
				unit = self.class_units.get(cls_name, {}).get(prop_name)
				if unit:
					return unit
			return ""

		return ""

	def _build_ui(self) -> None:
		main = ttk.Frame(self.root, padding=12)
		main.pack(fill="both", expand=True)

		file_frame = ttk.LabelFrame(main, text="Model Files", padding=10)
		file_frame.pack(fill="x")

		ttk.Label(file_frame, text="Input GLM:").grid(row=0, column=0, sticky="w", padx=4, pady=4)
		ttk.Entry(file_frame, textvariable=self.input_file_var, width=100).grid(
			row=0, column=1, sticky="ew", padx=4, pady=4
		)
		ttk.Button(file_frame, text="Browse...", command=self._pick_input_file).grid(
			row=0, column=2, padx=4, pady=4
		)

		ttk.Label(file_frame, text="Output File:").grid(row=1, column=0, sticky="w", padx=4, pady=4)
		ttk.Entry(file_frame, textvariable=self.output_file_var, width=100).grid(
			row=1, column=1, sticky="ew", padx=4, pady=4
		)
		ttk.Button(file_frame, text="Save As...", command=self._pick_output_file).grid(
			row=1, column=2, padx=4, pady=4
		)

		file_frame.columnconfigure(1, weight=1)

		options_frame = ttk.LabelFrame(main, text="Execution Options", padding=10)
		options_frame.pack(fill="x", pady=(8, 0))
		ttk.Label(options_frame, text="Random Seed (optional):").grid(
			row=0, column=0, sticky="w", padx=4, pady=4
		)
		ttk.Entry(options_frame, textvariable=self.seed_var, width=20).grid(
			row=0, column=1, sticky="w", padx=4, pady=4
		)
		ttk.Label(options_frame, text="Leave blank for non-deterministic sampling.").grid(
			row=0, column=2, sticky="w", padx=4, pady=4
		)

		rules_frame = ttk.LabelFrame(main, text="Property Edit Rules", padding=10)
		rules_frame.pack(fill="both", expand=True, pady=(10, 0))

		header = ttk.Frame(rules_frame)
		header.pack(fill="x")
		ttk.Label(header, text="Class Filter", width=16).grid(row=0, column=0, padx=4, sticky="w")
		ttk.Label(header, text="Property", width=34).grid(row=0, column=1, padx=4, sticky="w")
		ttk.Label(header, text="Min", width=12).grid(row=0, column=2, padx=4, sticky="w")
		ttk.Label(header, text="Max", width=12).grid(row=0, column=3, padx=4, sticky="w")
		ttk.Label(header, text="Units", width=16).grid(row=0, column=4, padx=4, sticky="w")
		ttk.Label(header, text="Histogram?", width=12).grid(row=0, column=5, padx=4, sticky="w")
		ttk.Label(header, text="", width=10).grid(row=0, column=6, padx=4, sticky="w")

		self.rows_container = ttk.Frame(rules_frame)
		self.rows_container.pack(fill="both", expand=True)

		rule_button_row = ttk.Frame(rules_frame)
		rule_button_row.pack(fill="x", pady=(8, 0))
		ttk.Button(rule_button_row, text="Add Property", command=self._add_rule_row).pack(side="left")
		ttk.Button(rule_button_row, text="Preview Affected Objects", command=self._preview_rules).pack(
			side="left", padx=(8, 0)
		)

		preview_frame = ttk.LabelFrame(main, text="Preview", padding=10)
		preview_frame.pack(fill="both", expand=True, pady=(10, 0))

		columns = ("class_filter", "property", "unit", "min", "max", "affected", "hist")
		self.preview_table = ttk.Treeview(preview_frame, columns=columns, show="headings", height=8)
		self.preview_table.heading("class_filter", text="Class Filter")
		self.preview_table.heading("property", text="Property")
		self.preview_table.heading("unit", text="Units")
		self.preview_table.heading("min", text="Min")
		self.preview_table.heading("max", text="Max")
		self.preview_table.heading("affected", text="Affected Objects")
		self.preview_table.heading("hist", text="Histogram?")
		self.preview_table.column("class_filter", width=120, anchor="w")
		self.preview_table.column("property", width=260, anchor="w")
		self.preview_table.column("unit", width=110, anchor="w")
		self.preview_table.column("min", width=90, anchor="e")
		self.preview_table.column("max", width=90, anchor="e")
		self.preview_table.column("affected", width=130, anchor="e")
		self.preview_table.column("hist", width=90, anchor="center")
		self.preview_table.pack(fill="both", expand=True)

		bottom = ttk.Frame(main)
		bottom.pack(fill="x", pady=(10, 0))

		ttk.Button(bottom, text="Export Edited Model", command=self._export_model).pack(side="left")
		ttk.Label(bottom, textvariable=self.status_var).pack(side="left", padx=12)

	def _add_rule_row(self) -> None:
		row = PropertyRuleRow(self.rows_container, self._remove_rule_row, self._metadata_provider)
		row.frame.pack(fill="x")
		self.rule_rows.append(row)

	def _remove_rule_row(self, row: PropertyRuleRow) -> None:
		if row in self.rule_rows:
			self.rule_rows.remove(row)

	def _pick_input_file(self) -> None:
		filename = filedialog.askopenfilename(
			title="Select GridLAB-D model file",
			filetypes=[("GridLAB-D Models", "*.glm"), ("All Files", "*.*")],
		)
		if filename:
			self.input_file_var.set(filename)
			if not self.output_file_var.get().strip():
				base = Path(filename)
				self.output_file_var.set(str(base.with_name(f"{base.stem}_edited{base.suffix}")))

	def _pick_output_file(self) -> None:
		filename = filedialog.asksaveasfilename(
			title="Save edited GridLAB-D model as",
			defaultextension=".glm",
			filetypes=[("GridLAB-D Models", "*.glm"), ("All Files", "*.*")],
		)
		if filename:
			self.output_file_var.set(filename)

	def _clear_preview_table(self) -> None:
		for row_id in self.preview_table.get_children():
			self.preview_table.delete(row_id)

	def _parse_seed(self) -> int | None:
		seed_text = self.seed_var.get().strip()
		if not seed_text:
			return None
		try:
			return int(seed_text)
		except ValueError as exc:
			raise ValueError("Random seed must be an integer.") from exc

	def _collect_rules(self) -> list[PropertyRule]:
		rules: list[PropertyRule] = []
		for row in self.rule_rows:
			if (
				not row.property_var.get().strip()
				and not row.min_var.get().strip()
				and not row.max_var.get().strip()
			):
				continue
			rules.append(row.get_rule())

		if not rules:
			raise ValueError("Add at least one property rule before preview/export.")

		return rules

	def _get_target_classes(self, class_filter: str) -> list[str]:
		return CLASS_FILTER_MAP[class_filter]

	def _collect_target_objects(self, sim, class_filter: str) -> list[str]:
		objects: list[str] = []
		seen: set[str] = set()
		for cls_name in self._get_target_classes(class_filter):
			try:
				names = sim.get_objects_by_class(cls_name)
			except Exception:
				names = []
			for name in names:
				if name not in seen:
					seen.add(name)
					objects.append(name)
		return objects

	def _load_sim(self, input_path: Path):
		sim = gridlabd.GridLabD()
		sim.set_working_directory(str(input_path.parent))
		sim.setup_before_load()
		sim.load_glm(str(input_path))
		sim.setup_after_load()
		return sim

	def _validate_paths(self) -> tuple[Path, Path]:
		input_text = self.input_file_var.get().strip()
		output_text = self.output_file_var.get().strip()
		if not input_text:
			raise ValueError("Please select an input GLM file.")
		if not output_text:
			raise ValueError("Please choose an output file.")

		input_path = Path(input_text)
		output_path = Path(output_text)
		if not input_path.exists():
			raise ValueError("Input GLM file does not exist.")
		return input_path, output_path

	def _preview_rules(self) -> None:
		sim = None
		try:
			input_path, _ = self._validate_paths()
			rules = self._collect_rules()
			self._parse_seed()

			self.status_var.set("Loading model for preview...")
			self.root.update_idletasks()

			sim = self._load_sim(input_path)
			self._clear_preview_table()

			for rule in rules:
				affected = 0
				target_objects = self._collect_target_objects(sim, rule.class_filter)
				for obj_name in target_objects:
					props = sim.get_object_properties(obj_name)
					if rule.property_name in props:
						affected += 1

				self.preview_table.insert(
					"",
					"end",
					values=(
						rule.class_filter,
						rule.property_name,
						rule.unit or "-",
						f"{rule.min_value:g}",
						f"{rule.max_value:g}",
						affected,
						"yes" if rule.show_histogram else "no",
					),
				)

			self.status_var.set("Preview updated.")
		except Exception as exc:
			self.status_var.set("Preview failed.")
			messagebox.showerror("Preview Error", str(exc))
		finally:
			if sim is not None:
				try:
					sim.finalize()
				except Exception:
					pass

	def _export_model(self) -> None:
		sim = None
		try:
			input_path, output_path = self._validate_paths()
			rules = self._collect_rules()
			seed = self._parse_seed()
			rng = random.Random(seed)

			self.status_var.set("Loading model...")
			self.root.update_idletasks()

			sim = self._load_sim(input_path)
			self._clear_preview_table()

			plot_data: dict[str, tuple[list[float], list[float]]] = {}
			total_updates = 0

			for rule in rules:
				original_vals: list[float] = []
				edited_vals: list[float] = []
				updates_for_rule = 0

				target_objects = self._collect_target_objects(sim, rule.class_filter)
				for obj_name in target_objects:
					props = sim.get_object_properties(obj_name)
					if rule.property_name not in props:
						continue

					original_num = _extract_numeric(props.get(rule.property_name))
					new_value = rng.uniform(rule.min_value, rule.max_value)
					sim.set_property(obj_name, rule.property_name, new_value)
					updates_for_rule += 1

					if original_num is not None:
						original_vals.append(original_num)
						edited_vals.append(new_value)

				total_updates += updates_for_rule
				self.preview_table.insert(
					"",
					"end",
					values=(
						rule.class_filter,
						rule.property_name,
						rule.unit or "-",
						f"{rule.min_value:g}",
						f"{rule.max_value:g}",
						updates_for_rule,
						"yes" if rule.show_histogram else "no",
					),
				)

				if rule.show_histogram and original_vals and edited_vals:
					plot_data[f"{rule.class_filter}:{rule.property_name}"] = (original_vals, edited_vals)

			if total_updates == 0:
				raise RuntimeError(
					"None of the requested class+property combinations matched any objects."
				)

			self.status_var.set("Exporting edited model...")
			self.root.update_idletasks()
			sim.save_checkpoint(str(output_path))

			self.status_var.set(f"Export complete: {output_path}")
			seed_display = "none" if seed is None else str(seed)
			messagebox.showinfo(
				"Export Complete",
				f"Edited model exported successfully to:\n{output_path}\n\n"
				f"Total updated object properties: {total_updates}\n"
				f"Random seed: {seed_display}",
			)

			if plot_data:
				self._show_histograms(plot_data)

		except Exception as exc:
			self.status_var.set("Export failed.")
			messagebox.showerror("Error", str(exc))
		finally:
			if sim is not None:
				try:
					sim.finalize()
				except Exception:
					pass

	def _show_histograms(self, plot_data: dict[str, tuple[list[float], list[float]]]) -> None:
		"""Show one histogram window per selected property when matplotlib is available."""
		try:
			import matplotlib.pyplot as plt
		except Exception:
			messagebox.showwarning(
				"Histogram Skipped",
				"matplotlib is not available in this Python environment, so histograms were skipped.",
			)
			return

		for prop_key, (orig, new) in plot_data.items():
			plt.figure(f"{prop_key} Distribution")
			plt.hist(orig, bins=20, alpha=0.6, label="Original")
			plt.hist(new, bins=20, alpha=0.6, label="New")
			plt.title(f"Property Distribution: {prop_key}")
			plt.xlabel(prop_key)
			plt.ylabel("Count")
			plt.legend()
			plt.tight_layout()

		plt.show()


def main() -> None:
	root = tk.Tk()
	ModelEditGUI(root)
	root.mainloop()


if __name__ == "__main__":
	main()