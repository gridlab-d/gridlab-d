"""
Created on 04/15/2026

On this special day in the USA (federal income taxes due), I've put together
this example test to validate the basic functionality of using the Python API
to run a transient analysis.

This test only defines a QSTS step size at 5 seconds.

Transient mode is triggered in the "data_cap_deltamode_trigger_2.player" file at
8.5 seconds and it must be completed prior to 10 seconds.

`get_unique_sorted_timestamps()` looks through all the message timestamps and 
returns a list of unique timestamps sorted in ascending order. This is printed
to console wherer the tester can evaluate them to ensure the model executed
QSTS (integer second) time steps and transient mode (non-integer second) time
steps at the expected times.

This tests use case #2 which is defined as the following sequence of events:
1. Run in QSTS to t=5
2. Call `step()` advancing to a target time of 10 seconds - Run QSTS from t=5
to t=8, and then trigger transient mode at t=8.5, returning to QSTS by t=10.

https://github.com/gridlab-d/gridlab-d/issues/1596

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""
import gridlabd
from pathlib import Path
import os
from datetime import datetime, timedelta
import pprint


def get_unique_sorted_timestamps(messages):
  """Return unique non-INIT message timestamps sorted in ascending order.

  Args:
    messages: List of dictionaries with keys "message", "timestamp", and
      "type".

  Returns:
    A list of unique timestamp strings sorted from earliest to latest.
  """
  unique_timestamps = {
    entry.get("timestamp")
    for entry in messages
    if isinstance(entry, dict) and entry.get("timestamp") not in (None, "INIT")
  }

  return sorted(
    unique_timestamps,
    key=lambda ts: datetime.fromisoformat(ts.replace("Z", "+00:00")),
  )

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)


gld = gridlabd.GridLabD(verbose=True)
model_path = Path("transient")
gld.set_working_directory(str(model_path))
load_status = gld.load("test_deltamode_capacitor_VAR_VOLT_ABC_indiv_dwell_NR.glm")
if load_status != 0:
    raise RuntimeError(f"Failed to load model with status code {load_status}")
start_str = gld.get_starttime()
starttime = datetime.fromisoformat(start_str)
print(f"Start time in model: {start_str}")

# Change player file to provide correct transient analysis 
# triggering for this scenario
gld.set_property("uselessdeltatrigger", "file", "data_cap_deltamode_trigger_2.player")

# Set-up step
gld.set_time_step(5)
error_code, return_time = gld.step()
if error_code != 0:
    raise RuntimeError(f"Error advancing time with `step()`; returned error code {error_code}")
print(f"Actual sim time reached is {return_time}")

# This step should trigger transient mode at 8.5 seconds and return to QSTS by 10 seconds
error_code, return_time = gld.step()
if error_code != 0:
    raise RuntimeError(f"Error advancing time with `step()`; returned error code {error_code}")
print(f"Actual sim time reached is {return_time}")


gld.stop()
messages = gld.get_messages()
sorted_message_timestamps = get_unique_sorted_timestamps(messages)
pprint(sorted_message_timestamps)
gld.exit_gld()



