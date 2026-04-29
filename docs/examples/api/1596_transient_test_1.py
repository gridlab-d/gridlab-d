"""
Created on 04/15/2026

On this special day in the USA (federal income taxes due), I've put together
this example test to validate the basic functionality of using the Python API
to run a transient analysis.

This is a test of the first internally-defined use case.

This test does not define a step size and uses defaults everywhere.

`get_unique_sorted_timestamps()` looks through all the message timestamps and 
returns a list of unique timestamps sorted in ascending order. This is printed
to console wherer the tester can evaluate them to ensure the model executed
QSTS (integer second) time steps and transient mode (non-integer second) time
steps at the expected times.

This tests use case #1 which is defined as the following sequence of events:
1. Run in QSTS to t=5
2. Call `step_to(5.5)` - Run transient mode from t=5 to t=5.5
3. Call `step_to(10.5)` - Run QSTS from t=6 to t=10, and then transient mode
  from t=10 to t=10.5

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


target_time_1 = starttime + timedelta(seconds=5.5)
target_time_1_str = target_time_1.isoformat()
print (f"Stepping to {target_time_1_str}")
error_code, return_time = gld.step_to(target_time_1_str)
if error_code != 0:
    raise RuntimeError(f"Error stepping to {target_time_1_str} with code {error_code}")
print(f"Target time is {target_time_1_str} and actual sim time reached was {return_time}")


target_time_2 = starttime + timedelta(seconds=10)
target_time_2_str = target_time_2.isoformat()
print (f"Stepping to {target_time_2_str}")
error_code, return_time = gld.step_to(target_time_2_str)
if error_code != 0:
    raise RuntimeError(f"Error stepping to {target_time_2_str} with code {error_code}")
print(f"Target time is {target_time_2_str} and actual sim time reached was {return_time}")


gld.stop()
messages = gld.get_messages()
sorted_message_timestamps = get_unique_sorted_timestamps(messages)
pprint(sorted_message_timestamps)
gld.exit_gld()



