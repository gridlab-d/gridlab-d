import os
import sys
from pathlib import Path
from datetime import datetime
import gridlabd
import matplotlib.pyplot as plt

from pyenergyplus.api import EnergyPlusAPI



script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir) 
step_size = 900
gld = gridlabd.GridLabD()
model_path = Path(script_dir)
gld.set_working_directory(str(model_path))
gld.load("houses_with_ep.glm")

EPLUS_HOME = Path("/usr/local/EnergyPlus-26-1-0")  # adjust as needed
IDF = Path(script_dir) / "RefBldgSmallHotelNew2004_Chicago.idf"
EPW = Path(script_dir) / "USA_WA_Pasco-Tri.Cities.AP.727845_TMY3.epw"
epload_name = "EPload"


def force_timestep_4_to_file(idf_in: Path, idf_out: Path):
    """Tiny text edit: replace existing Timestep object or add one."""
    txt = idf_in.read_text(encoding="utf-8", errors="ignore")
    lines = txt.splitlines()
    out = []
    replaced = False

    i = 0
    while i < len(lines):
        if lines[i].strip().lower().startswith("timestep,"):
            replaced = True
            out += ["Timestep,", "  4;  !- Number of Timesteps per Hour"]
            while i < len(lines) and ";" not in lines[i]:
                i += 1
            i += 1
            continue
        out.append(lines[i])
        i += 1

    if not replaced:
        out = ["Timestep,", "  4;  !- Number of Timesteps per Hour", ""] + out

    idf_out.write_text("\n".join(out) + "\n", encoding="utf-8")


def force_runperiod_july_only(idf_path: Path):
    """Replace existing RunPeriod object or add one for July 1 only."""
    txt = idf_path.read_text(encoding="utf-8", errors="ignore")
    lines = txt.splitlines()
    out = []
    replaced = False

    i = 0
    while i < len(lines):
        if lines[i].strip().lower().startswith("runperiod,"):
            replaced = True
            out += [
                "RunPeriod,",
                "  JulyFirst,                !- Name",
                "  7,                        !- Begin Month",
                "  1,                        !- Begin Day of Month",
                "  ,                         !- Begin Year",
                "  7,                        !- End Month",
                "  1,                        !- End Day of Month",
                "  ,                         !- End Year",
                "  Tuesday,                  !- Day of Week for Start Day",
                "  Yes,                      !- Use Weather File Holidays and Special Days",
                "  Yes,                      !- Use Weather File Daylight Saving Period",
                "  No,                       !- Apply Weekend Holiday Rule",
                "  Yes,                      !- Use Weather File Rain Indicators",
                "  Yes;                      !- Use Weather File Snow Indicators",
            ]
            while i < len(lines) and ";" not in lines[i]:
                i += 1
            i += 1
            continue
        out.append(lines[i])
        i += 1

    if not replaced:
        out = [
            "RunPeriod,",
            "  JulyFirst,                !- Name",
            "  7,                        !- Begin Month",
            "  1,                        !- Begin Day of Month",
            "  ,                         !- Begin Year",
            "  7,                        !- End Month",
            "  1,                        !- End Day of Month",
            "  ,                         !- End Year",
            "  Tuesday,                  !- Day of Week for Start Day",
            "  Yes,                      !- Use Weather File Holidays and Special Days",
            "  Yes,                      !- Use Weather File Daylight Saving Period",
            "  No,                       !- Apply Weekend Holiday Rule",
            "  Yes,                      !- Use Weather File Rain Indicators",
            "  Yes;                      !- Use Weather File Snow Indicators",
            "",
        ] + out

    idf_path.write_text("\n".join(out) + "\n", encoding="utf-8")


def update_ep_load(gld, new_load_kw):
    new_load_w = new_load_kw * 1000 # convert kW to W
    new_load_var = new_load_w * 0.6 # approx power factor of 0.85)
    return_code  = gld.set_property(object_name=epload_name, property_name="constant_power_A", value=str(new_load_w))
    gld.set_property(object_name=epload_name, property_name="constant_power_A", value=str(new_load_var))
    gld.set_property(object_name=epload_name, property_name="constant_power_B", value=str(new_load_w))
    gld.set_property(object_name=epload_name, property_name="constant_power_B", value=str(new_load_var))
    gld.set_property(object_name=epload_name, property_name="constant_power_C", value=str(new_load_w))
    gld.set_property(object_name=epload_name, property_name="constant_power_C", value=str(new_load_var))


def main():
    # Make pyenergyplus importable if you didn't install it into your interpreter
    sys.path.insert(0, str(EPLUS_HOME / "python"))

    api = EnergyPlusAPI()
    state = api.state_manager.new_state()

    # Prepare an output dir + IDF with 15-min timestep
    outdir = Path.cwd() / "out_5zone_runtime"
    outdir.mkdir(parents=True, exist_ok=True)
    idf_work = outdir / "in.idf"
    force_timestep_4_to_file(IDF, idf_work)
    force_runperiod_july_only(idf_work)

    # This list will be populated during runtime.
    # Each entry is a dictionary with calendar fields and value_j.
    ep_power_series = []
    gld_power_series = []
    callback_count = {"n": 0}
    value_source = {"name": None}

    # Cache variable/meter handles after they become available.
    h_facility_elec_w = {"h": None}
    h_facility_elec_meter = {"h": None}

    # Request an output variable known to exist for this model.
    api.exchange.request_variable(
        state,
        "Facility Total Purchased Electricity Energy",
        "WHOLE BUILDING",
    )

    def cb_begin_zone_timestep(state_arg):
        
        exch = api.exchange
        callback_count["n"] += 1

        # Variable/meter handles are only valid after API data is ready.
        if not exch.api_data_fully_ready(state_arg):
            return

        # Try to resolve the preferred output variable handle.
        if h_facility_elec_w["h"] is None:
            h = exch.get_variable_handle(
                state_arg,
                "Facility Total Purchased Electricity Energy",
                "WHOLE BUILDING",
            )
            if h >= 0:
                h_facility_elec_w["h"] = h
                value_source["name"] = "variable: Facility Total Purchased Electricity Energy"

        # Fallback: resolve a meter handle if the variable is unavailable.
        if h_facility_elec_w["h"] is None and h_facility_elec_meter["h"] is None:
            hm = exch.get_meter_handle(state_arg, "Electricity:Facility")
            if hm >= 0:
                h_facility_elec_meter["h"] = hm
                value_source["name"] = "meter: Electricity:Facility"

        if h_facility_elec_w["h"] is None and h_facility_elec_meter["h"] is None:
            return

        y = exch.year(state_arg)
        mo = exch.month(state_arg)
        d = exch.day_of_month(state_arg)
        hr = exch.hour(state_arg)       # 1-24
        mi = exch.minutes(state_arg)    # 0/15/30/45 for timestep=4

        if h_facility_elec_w["h"] is not None:
            value = exch.get_variable_value(state_arg, h_facility_elec_w["h"])
        else:
            value = exch.get_meter_value(state_arg, h_facility_elec_meter["h"])

        if mo == 7:  # only record July data
            ep_power_series.append(
                {
                    "year": y,
                    "month": mo,
                    "day": d,
                    "hour": hr,
                    "minute": mi,
                    "value_j": value,
                }
            )
            power = value/(15 * 60)/1000 # convert from J/15min to kW
            update_ep_load(gld, power)
            gld.step()
            return_code, gld_meter_power = gld.get_property(object_name="EPmeter", property_name="measured_real_power")
            gld_power_series.append(   
                {
                    "year": y,
                    "month": mo,
                    "day": d,
                    "hour": hr,
                    "minute": mi,
                    "value_j": gld_meter_power
                }
            )   




    # Register callback once
    api.runtime.callback_end_zone_timestep_after_zone_reporting(state, cb_begin_zone_timestep)

    # Run (NOTE: output dir is controlled by -d argument)
    args = [
        "-w", str(EPW),
        "-d", str(outdir),
        str(idf_work),
    ]
    rc = api.runtime.run_energyplus(state, args)

    api.state_manager.delete_state(state)

    if rc != 0:
        raise RuntimeError(f"EnergyPlus failed ({rc}). Check {outdir/'eplusout.err'}")

    # Now you can use the collected values as a normal Python variable
    print(f"Callback executions: {callback_count['n']}")
    if value_source["name"] is None:
        print("Warning: No variable or meter handle was resolved; check variable/meter names.")
    else:
        print(f"Data source: {value_source['name']}")

    # ep_power_series is your in-memory Python variable
    return ep_power_series, gld_power_series


def plot_series(ep_series, gld_series):
    """Plot EnergyPlus purchased electricity energy and GridLAB-D™ real power on a dual y-axis chart.

    Parameters:
    ep_series (list[dict]): EnergyPlus timestep records with keys year, month, day,
        hour, minute, value_j (Joules per 15-min timestep).
    gld_series (list[dict]): GridLAB-D™ timestep records with the same calendar keys
        and value_j holding measured real power in watts.
    """
    if not ep_series and not gld_series:
        print("No data to plot.")
        return

    sort_key = lambda rec: (rec["year"], rec["month"], rec["day"], rec["hour"], rec["minute"])
    ep_series = sorted(ep_series, key=sort_key)
    gld_series = sorted(gld_series, key=sort_key)

    def to_datetimes(series):
        timestamps = []
        for rec in series:
            try:
                ts = datetime(
                    rec["year"], rec["month"], rec["day"],
                    min(rec["hour"], 23), min(rec["minute"], 59),
                )
            except (ValueError, KeyError):
                continue
            timestamps.append(ts)
        return timestamps

    fig, ax1 = plt.subplots(figsize=(14, 6))

    if ep_series:
        ep_times = to_datetimes(ep_series)
        ep_values = [rec["value_j"] / 1000 for rec in ep_series]  # J -> kJ
        ax1.plot(ep_times, ep_values, color="tab:blue", label="EP Purchased Electricity (kJ)")
        ax1.set_ylabel("EnergyPlus Purchased Electricity Energy (kJ)", color="tab:blue")
        ax1.tick_params(axis="y", labelcolor="tab:blue")

    if gld_series:
        ax2 = ax1.twinx()
        gld_times = to_datetimes(gld_series)
        gld_values_kw = [rec["value_j"] / 1000 for rec in gld_series]  # W -> kW
        ax2.plot(gld_times, gld_values_kw, color="tab:orange", label="GLD Real Power (kW)")
        ax2.set_ylabel("GridLAB-D™ Measured Real Power (kW)", color="tab:orange")
        ax2.tick_params(axis="y", labelcolor="tab:orange")

    ax1.set_xlabel("Simulation Time")
    plt.title("EnergyPlus vs GridLAB-D™ Power")
    fig.legend(loc="upper left", bbox_to_anchor=(0.1, 0.9))
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    ep_series, gld_series = main()
    plot_series(ep_series, gld_series)