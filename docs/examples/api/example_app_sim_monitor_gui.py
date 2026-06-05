

"""Interactive GridLAB-D™ simulation monitor GUI demo.

This script launches a desktop GUI that runs a GridLAB-D™ model in steps and
plots selected object properties over time.

Usage:
		1. Run this script with the GridLAB-D™ Python environment active.
		2. Click Browse and select a GLM model file.
		3. Set start time, stop time, and step size.
		4. Add one or more plots in the Plots panel.
		5. Add one or more monitored parameters in Monitored Parameters:
			 - select an object,
			 - select or type a property,
			 - assign it to a plot.
		6. Click Apply Configuration.
		7. Use Start, Stop, Step Once, and Restart to control simulation playback.

Notes:
		- Restart resets simulation time to the configured start time and clears
			all plotted data.
		- Each parameter in a plot is drawn on its own y-axis.
"""

from __future__ import annotations

import re
import tkinter as tk
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Any

import matplotlib.dates as mdates
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from tkcalendar import DateEntry

import gridlabd


def _extract_numeric(value: Any) -> float:
	"""Convert a GridLAB-D™ value into a numeric value for plotting.

	Args:
		value: Raw value returned from GridLAB-D™ property APIs.

	Returns:
		A numeric scalar representation of the input value.

	Raises:
		RuntimeError: If no numeric component can be extracted.
	"""
	if isinstance(value, bool):
		return 1.0 if value else 0.0
	if isinstance(value, (int, float)):
		return float(value)
	if isinstance(value, complex):
		# Use magnitude for complex values so a single scalar can be plotted.
		return abs(value)

	text = str(value).strip()
	match = re.search(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", text)
	if not match:
		raise RuntimeError(f"Cannot plot non-numeric property value: {value!r}")
	return float(match.group(0))


def _parse_iso(value: str) -> datetime:
	"""Parse a GridLAB-D™ ISO timestamp into a naive local-wall-time datetime.

	GridLAB-D™ may return offset-aware values (e.g., ``-07:00``). GUI
	date/time picker inputs are naive datetimes. Normalize parsed values by
	stripping timezone info while preserving wall-clock fields so comparisons
	and arithmetic remain consistent.

	Args:
		value: ISO 8601 timestamp string.

	Returns:
		A timezone-naive datetime aligned to the timestamp wall-clock value.
	"""
	parsed = datetime.fromisoformat(value)
	if parsed.tzinfo is not None:
		return parsed.replace(tzinfo=None)
	return parsed


@dataclass
class PlotConfig:
	"""Configuration metadata for a plot container.

	Attributes:
		plot_id: Stable internal plot identifier.
		title: User-facing plot title.
	"""

	plot_id: str
	title: str


@dataclass
class SignalConfig:
	"""Configuration and data buffer for one monitored signal.

	Attributes:
		signal_id: Stable internal signal identifier.
		object_name: GridLAB-D™ object name.
		property_name: GridLAB-D™ property name.
		plot_id: Plot identifier this signal is assigned to.
		values: Collected numeric samples for plotting.
	"""

	signal_id: str
	object_name: str
	property_name: str
	plot_id: str
	values: list[float] = field(default_factory=list)


class SignalRow:
	"""Dynamic UI row representing one monitored signal selection."""

	def __init__(self, app: "SimulationMonitorApp", parent: tk.Widget, signal_id: str):
		"""Initialize a parameter row.

		Args:
			app: Parent application instance.
			parent: Tk container that will host this row.
			signal_id: Unique row identifier.
		"""
		self.app = app
		self.signal_id = signal_id
		self.frame = ttk.Frame(parent)

		self.object_var = tk.StringVar()
		self.property_var = tk.StringVar()
		self.plot_var = tk.StringVar()

		self.object_combo = ttk.Combobox(
			self.frame,
			textvariable=self.object_var,
			values=[],
			state="normal",
			width=28,
		)
		self.object_combo.grid(row=0, column=0, padx=4, pady=3, sticky="ew")

		# Editable combobox allows user to type properties not explicitly listed.
		self.property_combo = ttk.Combobox(
			self.frame,
			textvariable=self.property_var,
			values=[],
			state="normal",
			width=26,
		)
		self.property_combo.grid(row=0, column=1, padx=4, pady=3, sticky="ew")

		self.plot_combo = ttk.Combobox(
			self.frame,
			textvariable=self.plot_var,
			values=[],
			state="readonly",
			width=18,
		)
		self.plot_combo.grid(row=0, column=2, padx=4, pady=3, sticky="ew")

		remove_btn = ttk.Button(self.frame, text="Remove", command=self.remove)
		remove_btn.grid(row=0, column=3, padx=4, pady=3)

		self.object_combo.bind("<<ComboboxSelected>>", self._on_object_selected)
		self.object_combo.bind("<KeyRelease>", self._on_object_typed)
		self.property_combo.bind("<KeyRelease>", self._on_property_typed)

		# Allow clicking anywhere on the row to mark it as selected.
		for widget in (self.frame, self.object_combo, self.property_combo, self.plot_combo, remove_btn):
			widget.bind("<Button-1>", self._on_row_clicked, add="+")

	def _on_row_clicked(self, _event=None) -> None:
		"""Mark this row as selected in the parent UI.

		Args:
			_event: Tk click event, unused.
		"""
		self.app.select_signal_row(self.signal_id)

	def set_selected(self, selected: bool) -> None:
		"""Update visual selected state for this row.

		Args:
			selected: True to highlight row; False to clear highlight.
		"""
		self.frame.configure(relief="solid" if selected else "flat", borderwidth=1 if selected else 0)

	def remove(self) -> None:
		"""Remove this row from the UI and app state."""
		self.frame.destroy()
		self.app.remove_signal_row(self.signal_id)

	def _on_object_selected(self, _event=None) -> None:
		"""Refresh property choices after an object selection.

		Args:
			_event: Tk selection event, unused.
		"""
		self.refresh_property_choices()

	def _on_object_typed(self, _event=None) -> None:
		"""Filter object choices based on user typing.

		Args:
			_event: Tk key event, unused.
		"""
		typed = self.object_var.get().strip().lower()
		choices = self.app.get_object_names()
		if typed:
			filtered = [name for name in choices if typed in name.lower()]
			self.object_combo["values"] = filtered
		else:
			self.object_combo["values"] = choices

		obj = self.object_var.get().strip()
		if obj in choices:
			self.refresh_property_choices()
		else:
			self.property_combo["values"] = []

	def _on_property_typed(self, _event=None) -> None:
		"""Filter property choices based on user typing.

		Args:
			_event: Tk key event, unused.
		"""
		typed = self.property_var.get().strip().lower()
		choices = self.app.get_object_properties(self.object_var.get().strip())
		if typed:
			filtered = [name for name in choices if typed in name.lower()]
			self.property_combo["values"] = filtered
		else:
			self.property_combo["values"] = choices

	def refresh_object_choices(self) -> None:
		"""Refresh available object names from the application cache."""
		choices = self.app.get_object_names()
		self.object_combo["values"] = choices
		current = self.object_var.get().strip()
		if not current and choices:
			self.object_var.set(choices[0])
		self.refresh_property_choices()

	def refresh_property_choices(self) -> None:
		"""Refresh available properties for the currently selected object."""
		obj = self.object_var.get().strip()
		choices = self.app.get_object_properties(obj)
		self.property_combo["values"] = choices

		current = self.property_var.get().strip()
		if not current and choices:
			self.property_var.set(choices[0])

	def refresh_plot_choices(self) -> None:
		"""Refresh available plot assignments for this row."""
		plot_labels = self.app.get_plot_labels()
		self.plot_combo["values"] = plot_labels

		current = self.plot_var.get().strip()
		if current and current not in plot_labels:
			self.plot_var.set(plot_labels[0] if plot_labels else "")
		elif not current and plot_labels:
			self.plot_var.set(plot_labels[0])

	def to_config(self) -> SignalConfig:
		"""Validate row fields and build a SignalConfig instance.

		Returns:
			Validated signal configuration for this row.

		Raises:
			ValueError: If required fields are missing or invalid.
		"""
		object_name = self.object_var.get().strip()
		property_name = self.property_var.get().strip()
		plot_label = self.plot_var.get().strip()

		if not object_name:
			raise ValueError("Object name cannot be empty.")
		if not property_name:
			raise ValueError("Property name cannot be empty.")
		if not plot_label:
			raise ValueError("Each parameter must be assigned to a plot.")

		plot_id = self.app.plot_id_from_label(plot_label)
		if not plot_id:
			raise ValueError(f"Unknown plot selection: {plot_label}")

		return SignalConfig(
			signal_id=self.signal_id,
			object_name=object_name,
			property_name=property_name,
			plot_id=plot_id,
			values=[],
		)


class SimulationMonitorApp:
	"""Tkinter app for running GridLAB-D™ in steps and monitoring properties.

	This class owns all GUI widgets, model/session state, simulation control
	logic, and live plotting behavior.
	"""

	def __init__(self, root: tk.Tk):
		"""Initialize application state and construct the GUI.

		Args:
			root: Tk root window.
		"""
		self.root = root
		self.root.title("GridLAB-D™ Simulation Monitor")
		self.root.geometry("1400x980")
		self.root.minsize(1200, 860)

		self.gld: Any = None
		self.model_loaded = False
		self.model_path_var = tk.StringVar()
		now = datetime.now()
		self.start_hour_var = tk.StringVar(value=now.strftime("%H"))
		self.start_minute_var = tk.StringVar(value=now.strftime("%M"))
		self.start_second_var = tk.StringVar(value=now.strftime("%S"))
		self.stop_hour_var = tk.StringVar(value=(now + timedelta(hours=1)).strftime("%H"))
		self.stop_minute_var = tk.StringVar(value=(now + timedelta(hours=1)).strftime("%M"))
		self.stop_second_var = tk.StringVar(value=(now + timedelta(hours=1)).strftime("%S"))
		self.step_seconds_var = tk.StringVar(value="300")
		self.poll_interval_ms_var = tk.StringVar(value="200")
		self.history_limit_var = tk.StringVar(value="1000")
		self.status_var = tk.StringVar(value="Status: Select model to load")

		self.running = False
		self.after_id: str | None = None
		self.last_time: datetime | None = None
		self.config_start_time: datetime | None = None
		self.config_stop_time: datetime | None = None

		self.object_names: list[str] = []
		self.object_properties: dict[str, list[str]] = {}

		self.signal_rows: dict[str, SignalRow] = {}
		self.signal_counter = 0
		self.selected_signal_id: str | None = None

		self.plots: list[PlotConfig] = []
		self.plot_counter = 0

		self.time_data: list[datetime] = []
		self.signal_data: dict[str, SignalConfig] = {}

		self._build_ui()
		self._add_plot()
		self._add_signal_row()
		self.root.after(50, self._apply_default_layout)

		self.root.protocol("WM_DELETE_WINDOW", self._on_close)

	def _build_ui(self) -> None:
		"""Build all top-level UI regions and controls."""
		main = ttk.Frame(self.root)
		main.pack(fill=tk.BOTH, expand=True)

		controls = ttk.Frame(main)
		controls.pack(fill=tk.X, padx=10, pady=8)

		ttk.Label(controls, text="Model (GLM):").grid(row=0, column=0, sticky="w", padx=4, pady=4)
		ttk.Entry(controls, textvariable=self.model_path_var, width=90).grid(
			row=0, column=1, sticky="ew", padx=4, pady=4
		)
		ttk.Button(controls, text="Browse", command=self._browse_model).grid(
			row=0, column=2, sticky="ew", padx=4, pady=4
		)

		ttk.Label(controls, text="Start date/time:").grid(row=1, column=0, sticky="w", padx=4, pady=4)
		start_frame = ttk.Frame(controls)
		start_frame.grid(row=1, column=1, sticky="w", padx=4, pady=4)
		self.start_date_picker = DateEntry(start_frame, width=12, date_pattern="yyyy-mm-dd")
		self.start_date_picker.pack(side=tk.LEFT)
		ttk.Entry(start_frame, textvariable=self.start_hour_var, width=3).pack(side=tk.LEFT, padx=(8, 1))
		ttk.Label(start_frame, text=":").pack(side=tk.LEFT)
		ttk.Entry(start_frame, textvariable=self.start_minute_var, width=3).pack(side=tk.LEFT, padx=1)
		ttk.Label(start_frame, text=":").pack(side=tk.LEFT)
		ttk.Entry(start_frame, textvariable=self.start_second_var, width=3).pack(side=tk.LEFT, padx=1)

		ttk.Label(controls, text="Stop date/time:").grid(row=2, column=0, sticky="w", padx=4, pady=4)
		stop_frame = ttk.Frame(controls)
		stop_frame.grid(row=2, column=1, sticky="w", padx=4, pady=4)
		self.stop_date_picker = DateEntry(stop_frame, width=12, date_pattern="yyyy-mm-dd")
		default_stop_date = datetime.today() + timedelta(hours=1)
		self.stop_date_picker.set_date(default_stop_date)
		self.stop_date_picker.pack(side=tk.LEFT)
		ttk.Entry(stop_frame, textvariable=self.stop_hour_var, width=3).pack(side=tk.LEFT, padx=(8, 1))
		ttk.Label(stop_frame, text=":").pack(side=tk.LEFT)
		ttk.Entry(stop_frame, textvariable=self.stop_minute_var, width=3).pack(side=tk.LEFT, padx=1)
		ttk.Label(stop_frame, text=":").pack(side=tk.LEFT)
		ttk.Entry(stop_frame, textvariable=self.stop_second_var, width=3).pack(side=tk.LEFT, padx=1)

		ttk.Label(controls, text="Step (s):").grid(row=1, column=2, sticky="e", padx=4, pady=4)
		ttk.Entry(controls, textvariable=self.step_seconds_var, width=12).grid(
			row=1, column=3, sticky="w", padx=4, pady=4
		)

		ttk.Label(controls, text="Update every (ms):").grid(row=2, column=2, sticky="e", padx=4, pady=4)
		ttk.Entry(controls, textvariable=self.poll_interval_ms_var, width=10).grid(
			row=2, column=3, sticky="w", padx=4, pady=4
		)

		ttk.Label(controls, text="Max points:").grid(row=1, column=4, sticky="e", padx=4, pady=4)
		ttk.Entry(controls, textvariable=self.history_limit_var, width=10).grid(
			row=1, column=5, sticky="w", padx=4, pady=4
		)

		ttk.Button(controls, text="Start", command=self.start).grid(row=1, column=6, sticky="ew", padx=4, pady=4)
		ttk.Button(controls, text="Stop", command=self.stop).grid(row=1, column=7, sticky="ew", padx=4, pady=4)
		ttk.Button(controls, text="Restart", command=self.restart_simulation).grid(
			row=1, column=8, sticky="ew", padx=4, pady=4
		)
		ttk.Button(controls, text="Step Once", command=self.step_once).grid(
			row=2, column=6, sticky="ew", padx=4, pady=4
		)

		controls.columnconfigure(1, weight=1)

		self.content_pane = ttk.Panedwindow(main, orient=tk.VERTICAL)
		self.content_pane.pack(fill=tk.BOTH, expand=True, padx=10, pady=8)

		top = ttk.Frame(self.content_pane)
		bottom = ttk.Frame(self.content_pane)
		self.content_pane.add(top, weight=2)
		self.content_pane.add(bottom, weight=3)

		self._build_chart_panel(top)
		self._build_plots_panel(bottom)
		self._build_signals_panel(bottom)

		status = ttk.Label(main, textvariable=self.status_var, anchor="w")
		status.pack(fill=tk.X, padx=10, pady=(0, 8))

	def _apply_default_layout(self) -> None:
		"""Position panes so chart and parameter controls are both visible at startup."""
		try:
			total_h = self.content_pane.winfo_height()
			if total_h <= 1:
				return
			# Keep top chart compact enough to leave ample space for controls below.
			top_h = int(total_h * 0.42)
			self.content_pane.sashpos(0, top_h)
		except Exception:
			pass

	def _build_plots_panel(self, parent: ttk.Frame) -> None:
		"""Build the plot-management panel.

		Args:
			parent: Parent container for the panel.
		"""
		frame = ttk.LabelFrame(parent, text="Plots")
		frame.pack(fill=tk.X, padx=4, pady=(0, 8))

		self.plot_listbox = tk.Listbox(frame, height=6, exportselection=False)
		self.plot_listbox.pack(fill=tk.X, padx=6, pady=6)

		btns = ttk.Frame(frame)
		btns.pack(fill=tk.X, padx=6, pady=(0, 6))
		ttk.Button(btns, text="Add Plot", command=self._add_plot).pack(side=tk.LEFT, padx=3)
		ttk.Button(btns, text="Remove Selected Plot", command=self._remove_selected_plot).pack(side=tk.LEFT, padx=3)

	def _build_signals_panel(self, parent: ttk.Frame) -> None:
		"""Build the monitored-parameters panel.

		Args:
			parent: Parent container for the panel.
		"""
		frame = ttk.LabelFrame(parent, text="Monitored Parameters")
		frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

		controls = ttk.Frame(frame)
		controls.pack(fill=tk.X, padx=4, pady=(4, 4))
		ttk.Button(controls, text="Add Parameter", command=self._add_signal_row).pack(side=tk.LEFT, padx=3)
		ttk.Button(controls, text="Apply Configuration", command=self.apply_configuration).pack(side=tk.LEFT, padx=3)

		header = ttk.Frame(frame)
		header.pack(fill=tk.X, padx=4, pady=(4, 0))
		ttk.Label(header, text="Object", width=28).grid(row=0, column=0, padx=4, sticky="w")
		ttk.Label(header, text="Property", width=26).grid(row=0, column=1, padx=4, sticky="w")
		ttk.Label(header, text="Plot", width=18).grid(row=0, column=2, padx=4, sticky="w")

		list_container = ttk.Frame(frame)
		list_container.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

		canvas = tk.Canvas(list_container, borderwidth=0, highlightthickness=0)
		scrollbar = ttk.Scrollbar(list_container, orient="vertical", command=canvas.yview)
		self.signal_rows_container = ttk.Frame(canvas)

		self.signal_rows_container.bind(
			"<Configure>",
			lambda _e: canvas.configure(scrollregion=canvas.bbox("all")),
		)

		canvas.create_window((0, 0), window=self.signal_rows_container, anchor="nw")
		canvas.configure(yscrollcommand=scrollbar.set)

		canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
		scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

	def _build_chart_panel(self, parent: ttk.Frame) -> None:
		"""Build the Matplotlib chart panel.

		Args:
			parent: Parent container for the chart canvas.
		"""
		self.figure = Figure(figsize=(9, 4.8), dpi=100)
		self.canvas = FigureCanvasTkAgg(self.figure, master=parent)
		self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

	def _browse_model(self) -> None:
		"""Open a model-file picker and auto-load the selected GLM."""
		selected = filedialog.askopenfilename(
			title="Select GridLAB-D™ model",
			filetypes=[("GridLAB-D™ model", "*.glm"), ("All files", "*.*")],
		)
		if selected:
			self.model_path_var.set(selected)
			self._load_model()

	def _load_model(self) -> None:
		"""Load the selected model file and refresh object/property metadata.

		Raises:
			RuntimeError: Wrapped as dialog errors for model load failures.
		"""
		path_text = self.model_path_var.get().strip()
		if not path_text:
			messagebox.showerror("Missing model", "Please choose a GLM file first.")
			return

		model_path = Path(path_text).expanduser().resolve()
		if not model_path.exists():
			messagebox.showerror("Model not found", f"File does not exist:\n{model_path}")
			return

		self.stop()
		self._shutdown_gld()

		try:
			self.gld = gridlabd.GridLabD()
			self.gld.set_working_directory(str(model_path.parent))
			# Use pythonic signature to load a single file.
			self.gld.load_glm(str(model_path))
			self._refresh_model_metadata()
			self._apply_time_bounds()
			self.model_loaded = True
			self.apply_configuration()
			self.status_var.set(
				f"Status: Loaded {model_path.name} | Objects discovered: {len(self.object_names)}"
			)
		except Exception as exc:
			self.model_loaded = False
			self.gld = None
			messagebox.showerror("Load failed", f"Failed to load model:\n{exc}")

	def _refresh_model_metadata(self) -> None:
		"""Refresh object and property lookup caches from GridLAB-D™ APIs.

		Raises:
			RuntimeError: If metadata cannot be retrieved or no objects are found.
		"""
		if not self.gld:
			return

		try:
			classes = self.gld.get_all_classes()
			object_names: list[str] = []

			for class_name in classes:
				try:
					names = self.gld.get_objects_by_class(class_name)
					normalized = [str(name).strip() for name in names if str(name).strip()]
					object_names.extend(normalized)
				except Exception:
					# Some classes may have no instances in the active model.
					continue

			self.object_names = sorted(set(object_names))
			if not self.object_names:
				raise RuntimeError(
					"No objects were returned by get_all_classes/get_objects_by_class for this model."
				)

			self.object_properties = {}

			for object_name in self.object_names:
				try:
					props = self.gld.get_object_properties(object_name)
					self.object_properties[object_name] = sorted(list(props.keys()))
				except Exception:
					self.object_properties[object_name] = []

			for row in self.signal_rows.values():
				row.refresh_object_choices()
		except Exception as exc:
			raise RuntimeError(f"Failed to query object metadata: {exc}") from exc

	def get_object_names(self) -> list[str]:
		"""Return available object names for object-combobox controls.

		Returns:
			Sorted object names discovered from the loaded model.
		"""
		return self.object_names

	def get_object_properties(self, object_name: str) -> list[str]:
		"""Return available property names for a specific object.

		Args:
			object_name: GridLAB-D™ object name.

		Returns:
			Property names for the requested object, or an empty list.
		"""
		if not object_name:
			return []
		return self.object_properties.get(object_name, [])

	def get_plot_labels(self) -> list[str]:
		"""Return plot labels for combobox display.

		Returns:
			Labels in the form "Px: Title".
		"""
		return [f"{p.plot_id}: {p.title}" for p in self.plots]

	def plot_id_from_label(self, label: str) -> str | None:
		"""Resolve internal plot ID from a combobox label.

		Args:
			label: Display label in the form "Px: Title".

		Returns:
			Matching plot ID, or None if no match is found.
		"""
		for plot in self.plots:
			full = f"{plot.plot_id}: {plot.title}"
			if label == full:
				return plot.plot_id
		return None

	def _add_plot(self) -> None:
		"""Create a new plot entry and refresh related UI state."""
		self.plot_counter += 1
		plot_id = f"P{self.plot_counter}"
		title = f"Plot {self.plot_counter}"
		self.plots.append(PlotConfig(plot_id=plot_id, title=title))
		self._refresh_plot_listbox()
		self._refresh_all_plot_choices()
		self._redraw()

	def _remove_selected_plot(self) -> None:
		"""Remove the selected plot and reassign dependent signals."""
		if len(self.plots) <= 1:
			messagebox.showwarning("Cannot remove", "At least one plot must remain.")
			return

		sel = self.plot_listbox.curselection()
		if not sel:
			messagebox.showinfo("No selection", "Select a plot to remove.")
			return

		index = sel[0]
		removed = self.plots.pop(index)
		replacement = self.plots[0].plot_id

		for cfg in self.signal_data.values():
			if cfg.plot_id == removed.plot_id:
				cfg.plot_id = replacement

		for row in self.signal_rows.values():
			row.refresh_plot_choices()

		self._refresh_plot_listbox()
		self._redraw()

	def _refresh_plot_listbox(self) -> None:
		"""Refresh listbox display of plot definitions."""
		self.plot_listbox.delete(0, tk.END)
		for plot in self.plots:
			self.plot_listbox.insert(tk.END, f"{plot.plot_id}: {plot.title}")

	def _refresh_all_plot_choices(self) -> None:
		"""Refresh plot-combobox choices for all parameter rows."""
		for row in self.signal_rows.values():
			row.refresh_plot_choices()

	def _add_signal_row(self) -> None:
		"""Add one monitored-parameter row to the UI."""
		self.signal_counter += 1
		signal_id = f"S{self.signal_counter}"
		row = SignalRow(self, self.signal_rows_container, signal_id)
		self.signal_rows[signal_id] = row
		row.frame.pack(fill=tk.X, padx=2, pady=1)

		row.refresh_object_choices()
		row.refresh_plot_choices()
		self.select_signal_row(signal_id)

	def select_signal_row(self, signal_id: str) -> None:
		"""Mark a parameter row as selected.

		Args:
			signal_id: Row identifier to select.
		"""
		if signal_id not in self.signal_rows:
			return
		self.selected_signal_id = signal_id
		for sid, row in self.signal_rows.items():
			row.set_selected(sid == signal_id)

	def _remove_selected_signal(self) -> None:
		"""Remove the selected parameter row or the newest row as fallback."""
		if not self.signal_rows:
			messagebox.showinfo("No parameters", "There are no parameters to remove.")
			return

		target_id = self.selected_signal_id
		if not target_id or target_id not in self.signal_rows:
			# Fallback: remove most recently added if nothing is selected.
			target_id = next(reversed(self.signal_rows))

		self.signal_rows[target_id].remove()

	def remove_signal_row(self, signal_id: str) -> None:
		"""Remove a parameter row from app state and refresh selection.

		Args:
			signal_id: Row identifier to remove.
		"""
		if signal_id in self.signal_rows:
			del self.signal_rows[signal_id]
		if signal_id in self.signal_data:
			del self.signal_data[signal_id]
		if self.selected_signal_id == signal_id:
			self.selected_signal_id = None
			if self.signal_rows:
				next_id = next(iter(self.signal_rows))
				self.select_signal_row(next_id)
		self._redraw()

	def apply_configuration(self) -> None:
		"""Validate all rows and commit the monitoring configuration.

		Raises:
			ValueError: Surfaced as dialog errors if row configuration is invalid.
		"""
		try:
			new_data: dict[str, SignalConfig] = {}
			for signal_id, row in self.signal_rows.items():
				cfg = row.to_config()
				existing = self.signal_data.get(signal_id)
				if existing:
					cfg.values = existing.values
				new_data[signal_id] = cfg
			self.signal_data = new_data
			self.status_var.set("Status: Parameter configuration applied")
			self._redraw()
		except ValueError as exc:
			messagebox.showerror("Invalid configuration", str(exc))

	def _step_seconds(self) -> int:
		"""Parse and validate the simulation step size.

		Returns:
			Step size in seconds.

		Raises:
			ValueError: If the value is not a positive integer.
		"""
		try:
			value = int(self.step_seconds_var.get().strip())
			if value <= 0:
				raise ValueError
			return value
		except ValueError as exc:
			raise ValueError("Step size must be a positive integer number of seconds.") from exc

	def _poll_interval_ms(self) -> int:
		"""Parse and validate GUI update interval.

		Returns:
			Polling interval in milliseconds.

		Raises:
			ValueError: If interval is below minimum or invalid.
		"""
		try:
			value = int(self.poll_interval_ms_var.get().strip())
			if value < 10:
				raise ValueError
			return value
		except ValueError as exc:
			raise ValueError("Update interval must be an integer >= 10 ms.") from exc

	def _history_limit(self) -> int:
		"""Parse and validate time-series buffer length.

		Returns:
			Maximum stored point count.

		Raises:
			ValueError: If value is below minimum or invalid.
		"""
		try:
			value = int(self.history_limit_var.get().strip())
			if value < 10:
				raise ValueError
			return value
		except ValueError as exc:
			raise ValueError("Max points must be an integer >= 10.") from exc

	def _time_parts(self, hour_var: tk.StringVar, minute_var: tk.StringVar, second_var: tk.StringVar, field_name: str) -> tuple[int, int, int]:
		"""Parse and validate HH:MM:SS values from Tk variables.

		Args:
			hour_var: Tk variable containing hour text.
			minute_var: Tk variable containing minute text.
			second_var: Tk variable containing second text.
			field_name: Human-readable field label for errors.

		Returns:
			Tuple of validated (hour, minute, second).

		Raises:
			ValueError: If values are non-integer or out of range.
		"""
		try:
			hour = int(hour_var.get().strip())
			minute = int(minute_var.get().strip())
			second = int(second_var.get().strip())
		except ValueError as exc:
			raise ValueError(f"{field_name} time must be integers HH:MM:SS.") from exc

		if not (0 <= hour <= 23):
			raise ValueError(f"{field_name} hour must be in 0..23.")
		if not (0 <= minute <= 59):
			raise ValueError(f"{field_name} minute must be in 0..59.")
		if not (0 <= second <= 59):
			raise ValueError(f"{field_name} second must be in 0..59.")

		return hour, minute, second

	def _get_user_datetime(self, which: str) -> datetime:
		"""Build a datetime from date-picker and time-entry controls.

		Args:
			which: Either "start" or "stop".

		Returns:
			Composed datetime for the requested endpoint.

		Raises:
			ValueError: If date/time inputs are invalid.
		"""
		if which == "start":
			date_value = self.start_date_picker.get_date()
			hour, minute, second = self._time_parts(
				self.start_hour_var,
				self.start_minute_var,
				self.start_second_var,
				"Start time",
			)
		else:
			date_value = self.stop_date_picker.get_date()
			hour, minute, second = self._time_parts(
				self.stop_hour_var,
				self.stop_minute_var,
				self.stop_second_var,
				"Stop time",
			)

		try:
			return datetime(
				date_value.year,
				date_value.month,
				date_value.day,
				hour,
				minute,
				second,
			)
		except ValueError as exc:
			raise ValueError(f"Invalid {which} date/time selection.") from exc

	def _apply_time_bounds(self) -> None:
		"""Apply user-configured start and stop times to GridLAB-D™.

		Raises:
			ValueError: If stop time is not later than start time.
		"""
		if not self.gld:
			return

		start_dt = self._get_user_datetime("start")
		stop_dt = self._get_user_datetime("stop")

		if start_dt and stop_dt and stop_dt <= start_dt:
			raise ValueError("Stop time must be later than start time.")

		if start_dt is not None:
			self.gld.set_starttime(start_dt.isoformat())
		if stop_dt is not None:
			self.gld.set_stoptime(stop_dt.isoformat())

		self.config_start_time = start_dt
		self.config_stop_time = stop_dt

	def start(self) -> None:
		"""Start periodic simulation stepping and live plot updates."""
		if not self.model_loaded or not self.gld:
			messagebox.showerror("No model", "Load a model before starting the simulation.")
			return

		try:
			self.apply_configuration()
			self._apply_time_bounds()
			_ = self._step_seconds()
			_ = self._poll_interval_ms()
			_ = self._history_limit()
		except ValueError as exc:
			messagebox.showerror("Invalid settings", str(exc))
			return

		if self.running:
			return

		self.running = True
		self.status_var.set("Status: Running")
		self._schedule_next_step()

	def stop(self) -> None:
		"""Stop periodic simulation stepping."""
		self.running = False
		if self.after_id is not None:
			self.root.after_cancel(self.after_id)
			self.after_id = None
		if self.model_loaded:
			self.status_var.set("Status: Stopped")

	def _clear_plot_data(self) -> None:
		"""Clear all collected time-series data and redraw empty plots."""
		self.time_data.clear()
		for cfg in self.signal_data.values():
			cfg.values.clear()
		self.last_time = None
		self._redraw()

	def restart_simulation(self) -> None:
		"""Restart simulation from start time and clear existing plotted data."""
		if not self.model_loaded or not self.gld:
			messagebox.showerror("No model", "Load a model before restarting the simulation.")
			return

		was_running = self.running
		self.stop()

		try:
			self.apply_configuration()
			self._apply_time_bounds()

			if self.config_start_time is None:
				raise ValueError("Start time is not configured.")

			code = self.gld.set_time(self.config_start_time.isoformat())
			if code != 0:
				raise RuntimeError(f"GridLAB-D™ set_time failed with code {code}")

			self._clear_plot_data()
			self.status_var.set(
				f"Status: Restarted at {self.config_start_time.isoformat()}"
			)

			if was_running:
				self.start()
		except Exception as exc:
			messagebox.showerror("Restart failed", str(exc))

	def step_once(self) -> None:
		"""Execute one simulation step and refresh plots."""
		if not self.model_loaded or not self.gld:
			messagebox.showerror("No model", "Load a model before stepping.")
			return

		try:
			self.apply_configuration()
			self._apply_time_bounds()
			self._perform_step()
		except Exception as exc:
			self.stop()
			messagebox.showerror("Runtime error", str(exc))

	def _schedule_next_step(self) -> None:
		"""Run one step cycle and schedule the next cycle."""
		if not self.running:
			return

		try:
			self._perform_step()
		except Exception as exc:
			self.stop()
			messagebox.showerror("Runtime error", str(exc))
			return

		interval = self._poll_interval_ms()
		self.after_id = self.root.after(interval, self._schedule_next_step)

	def _perform_step(self) -> None:
		"""Advance simulation to the next target time and collect signal values.

		Raises:
			RuntimeError: If GridLAB-D™ calls fail or return invalid states.
		"""
		if not self.gld:
			raise RuntimeError("Simulation is not initialized.")

		step_size = self._step_seconds()
		history_limit = self._history_limit()

		code, current_time_str = self.gld.get_time()
		if code != 0:
			raise RuntimeError(f"GridLAB-D™ get_time failed with code {code}")

		if current_time_str is None:
			# Initialization state: do one step first to get a concrete timestamp.
			step_code, stepped_time = self.gld.step()
			if step_code != 0:
				raise RuntimeError(f"GridLAB-D™ step failed with code {step_code}")
			if stepped_time is None:
				raise RuntimeError("GridLAB-D™ returned no simulation time after step.")
			sim_time = _parse_iso(stepped_time)
		else:
			base_time = _parse_iso(current_time_str)
			if self.config_stop_time is not None and base_time >= self.config_stop_time:
				self.stop()
				self.status_var.set(
					f"Status: Reached stop time {self.config_stop_time.isoformat()}"
				)
				return

			target_time = base_time + timedelta(seconds=step_size)
			if self.config_stop_time is not None and target_time > self.config_stop_time:
				target_time = self.config_stop_time

			step_code, stepped_time = self.gld.step_to(target_time.isoformat())
			if step_code != 0:
				raise RuntimeError(f"GridLAB-D™ step_to failed with code {step_code}")
			if stepped_time is None:
				raise RuntimeError("GridLAB-D™ returned no simulation time after step_to.")
			sim_time = _parse_iso(stepped_time)

		self.time_data.append(sim_time)
		if len(self.time_data) > history_limit:
			self.time_data = self.time_data[-history_limit:]

		for signal_id, cfg in self.signal_data.items():
			value = self.gld.get_object_property_value(cfg.object_name, cfg.property_name)
			numeric_value = _extract_numeric(value)
			cfg.values.append(numeric_value)
			if len(cfg.values) > history_limit:
				cfg.values[:] = cfg.values[-history_limit:]

			if len(cfg.values) < len(self.time_data):
				pad_count = len(self.time_data) - len(cfg.values)
				cfg.values[:0] = [cfg.values[0]] * pad_count

		self.last_time = sim_time
		if self.config_stop_time is not None and sim_time >= self.config_stop_time:
			self.stop()
			self.status_var.set(
				f"Status: Reached stop time {self.config_stop_time.isoformat()}"
			)
		else:
			self.status_var.set(f"Status: Running | Sim time {sim_time.isoformat()}")
		self._redraw()

	def _redraw(self) -> None:
		"""Redraw all plot panels from current in-memory data buffers."""
		self.figure.clear()

		if not self.plots:
			self.canvas.draw_idle()
			return

		nplots = len(self.plots)
		axes = self.figure.subplots(nplots, 1, squeeze=False)

		for idx, plot in enumerate(self.plots):
			base_ax = axes[idx][0]
			base_ax.grid(True, alpha=0.25)
			base_ax.set_title(plot.title)

			assigned = [cfg for cfg in self.signal_data.values() if cfg.plot_id == plot.plot_id]

			if not assigned:
				base_ax.text(0.5, 0.5, "No parameters assigned", ha="center", va="center", transform=base_ax.transAxes)
				base_ax.set_ylabel("-")
				continue

			for param_index, cfg in enumerate(assigned):
				if param_index == 0:
					ax = base_ax
				else:
					ax = base_ax.twinx()
					ax.spines["right"].set_position(("axes", 1.0 + 0.12 * (param_index - 1)))

				label = f"{cfg.object_name}.{cfg.property_name}"
				series = cfg.values
				points = min(len(self.time_data), len(series))

				if points > 0:
					x = self.time_data[-points:]
					y = series[-points:]
					color = f"C{param_index % 10}"
					ax.plot(x, y, color=color, linewidth=1.8)
					ax.set_ylabel(label, color=color)
					ax.tick_params(axis="y", colors=color)
				else:
					ax.set_ylabel(label)

			base_ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d %H:%M:%S"))

		self.figure.tight_layout()
		self.canvas.draw_idle()

	def _shutdown_gld(self) -> None:
		"""Finalize and release the active GridLAB-D™ instance if present."""
		if not self.gld:
			return
		try:
			self.gld.finalize()
		except Exception:
			pass
		self.gld = None

	def _on_close(self) -> None:
		"""Handle GUI window close event with graceful cleanup."""
		self.stop()
		self._shutdown_gld()
		self.root.destroy()


def main() -> None:
	"""Run the simulation monitor GUI application."""
	root = tk.Tk()
	app = SimulationMonitorApp(root)
	app._redraw()
	root.mainloop()


if __name__ == "__main__":
	main()