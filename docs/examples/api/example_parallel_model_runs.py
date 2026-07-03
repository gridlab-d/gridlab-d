
"""
Run multiple GridLAB-D™ models in parallel and compute daily energy usage.

This script launches one process per model folder using Python multiprocessing,
collects measured_real_power from object network_node at 300-second intervals,
and reports per-model plus fleet-average daily energy.
"""

from __future__ import annotations

import multiprocessing as mp
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
import re

import gridlabd


STEP_SECONDS = 300
SIM_START = datetime(2026, 7, 1, 0, 0, 0)
SIM_END = datetime(2026, 7, 2, 0, 0, 0)
MODEL_FOLDERS = ["house_with_solar", "house_with_solar2", "house_with_solar3"]


@dataclass
class RunResult:
	model_name: str
	success: bool
	energy_kwh: float | None
	samples: list[tuple[str, float]]
	error: str | None = None


def _parse_real_power_watts(raw_value: str | float | int) -> float:
	"""Convert GridLAB-D™ measured_real_power value into watts."""
	if isinstance(raw_value, (int, float)):
		return float(raw_value)

	text = str(raw_value).strip()
	if not text:
		raise ValueError("Empty measured_real_power value")

	unit_factor = 1.0
	lowered = text.lower()
	if " mw" in lowered:
		unit_factor = 1_000_000.0
	elif " kw" in lowered:
		unit_factor = 1_000.0
	elif " w" in lowered:
		unit_factor = 1.0

	token = text.split()[0].replace("i", "j")
	try:
		return complex(token).real * unit_factor
	except ValueError:
		match = re.search(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", token)
		if not match:
			raise ValueError(f"Cannot parse measured_real_power value: {text!r}")
		return float(match.group(0)) * unit_factor


def _run_single_model(model_dir: str) -> RunResult:
	"""Run one model for one day and return sampled power and total energy."""
	model_path = Path(model_dir).resolve()
	gld = gridlabd.GridLabD()
	samples: list[tuple[str, float]] = []

	try:
		gld.set_working_directory(str(model_path))
		load_code = gld.load("houses.glm")
		if load_code != 0:
			raise RuntimeError(f"Failed to load houses.glm in {model_path}")

		# Apply runtime clock bounds for the requested one-day simulation.
		# global_setvar return codes vary by build; read back time instead.
		gld.global_setvar("starttime", SIM_START.isoformat(sep=" "))
		gld.global_setvar("stoptime", SIM_END.isoformat(sep=" "))

		gld.set_time_step(STEP_SECONDS)
		gld.setup_after_load()
		code, start_time = gld.get_time()
		if code != 0:
			raise RuntimeError(f"get_time failed in {model_path.name} with code {code}")
		if start_time is None or not start_time.startswith("2026-07-01T00:00:00"):
			raise RuntimeError(
				f"Clock override failed in {model_path.name}; expected 2026-07-01T00:00:00, got {start_time!r}"
			)

		total_steps = int((SIM_END - SIM_START).total_seconds() / STEP_SECONDS)
		for _ in range(total_steps):
			step_code, stepped_time = gld.step()
			if step_code != 0:
				raise RuntimeError(
					f"step failed for {model_path.name} with code {step_code}"
				)

			prop_code, prop_value = gld.get_property("network_node", "measured_real_power")
			if prop_code != 0:
				raise RuntimeError(
					f"get_property failed for {model_path.name} at {stepped_time} with code {prop_code}"
				)

			real_power_w = _parse_real_power_watts(prop_value)
			samples.append((stepped_time, real_power_w))

		energy_wh = sum(power_w * STEP_SECONDS / 3600.0 for _, power_w in samples)
		energy_kwh = energy_wh / 1000.0
		return RunResult(
			model_name=model_path.name,
			success=True,
			energy_kwh=energy_kwh,
			samples=samples,
		)
	except Exception as exc:
		return RunResult(
			model_name=model_path.name,
			success=False,
			energy_kwh=None,
			samples=samples,
			error=str(exc),
		)
	finally:
		try:
			gld.stop()
		except Exception:
			pass
		try:
			gld.exit_gld()
		except Exception:
			pass


def main() -> None:
	script_dir = Path(__file__).resolve().parent
	model_paths = [str((script_dir / name).resolve()) for name in MODEL_FOLDERS]

	missing = [path for path in model_paths if not Path(path).is_dir()]
	if missing:
		raise FileNotFoundError(f"Model folders not found: {missing}")

	ctx = mp.get_context("spawn")
	with ctx.Pool(processes=len(model_paths)) as pool:
		results: list[RunResult] = pool.map(_run_single_model, model_paths)

	results.sort(key=lambda item: item.model_name)
	failed_results = [item for item in results if not item.success]
	successful_results = [item for item in results if item.success]

	print(f"Simulation window: {SIM_START.isoformat()} to {SIM_END.isoformat()}")
	print(f"Time step: {STEP_SECONDS} seconds")
	print("\nDaily real energy consumption by model:")
	for item in results:
		if item.success:
			print(
				f"  {item.model_name}: {item.energy_kwh:.6f} kWh "
				f"({len(item.samples)} measured_real_power samples)"
			)
		else:
			print(
				f"  {item.model_name}: FAILED "
				f"after {len(item.samples)} measured_real_power samples"
			)
			print(f"    error: {item.error}")

	if failed_results:
		print("\nAverage not computed because one or more model runs failed.")
	elif successful_results:
		avg_kwh = sum(item.energy_kwh for item in successful_results if item.energy_kwh is not None) / len(successful_results)
		print(f"\nThree-circuit average daily real energy: {avg_kwh:.6f} kWh")


if __name__ == "__main__":
	main()