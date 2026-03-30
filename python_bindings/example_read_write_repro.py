"""Minimal repro runner for example_read_write_script without plotting deps."""

from datetime import datetime, timedelta, timezone
from pathlib import Path
import os

import gridlabd


def parse_float(value):
    text = str(value)
    parts = text.split()
    token = parts[0] if parts else text
    try:
        return float(token)
    except Exception:
        num = ""
        saw_dot = False
        saw_sign = False
        for ch in token:
            if ch.isdigit():
                num += ch
                continue
            if ch in "+-" and not saw_sign and not num:
                num += ch
                saw_sign = True
                continue
            if ch == "." and not saw_dot:
                num += ch
                saw_dot = True
                continue
        return float(num) if num not in ("", "+", "-") else 0.0


def parse_value_dict(data):
    return {k: parse_float(v) for k, v in data.items()}


script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

gld = gridlabd.GridLabD()
gld.set_working_directory(str(Path("house_with_solar")))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"load failed: {load_code}")

starttime = datetime.fromisoformat(gld.get_starttime()).replace(
    tzinfo=timezone(timedelta(hours=-7))
)
stop_time_obj = starttime + timedelta(days=2)
gld.set_stoptime(datetime.isoformat(stop_time_obj))

code, sim_time = gld.get_time()
if code != 0:
    raise RuntimeError(f"get_time failed: {code}, {sim_time}")
sim_time_obj = datetime.fromisoformat(sim_time)

inverter_rated_power = parse_value_dict(gld.get_properties_by_class("inverter", "rated_power"))
original_cooling = parse_value_dict(gld.get_properties_by_class("house", "cooling_setpoint"))

step_index = 0
while sim_time_obj < stop_time_obj:
    try:
        code, sim_time = gld.step()
    except Exception as ex:
        print(f"EXCEPTION during step at index={step_index}, time={sim_time_obj.isoformat()}: {ex}")
        raise

    if code != 0:
        print(f"Non-zero step code at index={step_index}: code={code}, time={sim_time}")
        break

    sim_time_obj = datetime.fromisoformat(sim_time)
    if step_index % 100 == 0:
        print(f"step={step_index} time={sim_time_obj.isoformat()}")

    cooling = parse_value_dict(gld.get_properties_by_class("house", "cooling_setpoint"))
    solar = parse_value_dict(gld.get_properties_by_class("inverter", "VA_Out"))

    for house, original_sp in original_cooling.items():
        inverter_name = f"{house}_sol_inverter"
        solar_power = solar.get(inverter_name, 0.0)
        rated_power = inverter_rated_power.get(inverter_name, 0.0)
        _ = cooling.get(house, original_sp)
        if rated_power > 0 and solar_power > 0.5 * rated_power:
            gld.set_property(house, "cooling_setpoint", "60")
        else:
            gld.set_property(house, "cooling_setpoint", str(original_sp))

    step_index += 1

print("Stopping model")
gld.stop()
gld.exit_gld()
print("Done")
