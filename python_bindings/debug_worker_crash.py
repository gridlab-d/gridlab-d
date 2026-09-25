#!/usr/bin/env python3
"""Debug script to reproduce worker crash after step() completes."""

import gridlabd
import os

model_path = os.path.join(os.path.dirname(__file__), "tests/test_HVAC_balance.glm")

print("Creating GridLabD instance...")
gld = gridlabd.GridLabD()

print(f"Loading model: {model_path}")
result = gld.load(model_path)
print(f"Load result: {result}")

print("\nStepping simulation...")
for i in range(30):
    try:
        result, sim_time = gld.step()
        print(f"  Step {i}: result={result}, time={sim_time}")
        if result != 0:
            print(f"  Simulation ended at step {i}")
            break
    except Exception as e:
        print(f"  Exception at step {i}: {e}")
        break

print("\nTrying to query objects after simulation...")
try:
    classes = gld.get_all_classes()
    print(f"  Classes: {classes}")
    
    for cls in classes:
        print(f"  Getting objects for class: {cls}")
        objs = gld.get_all_objects(cls)
        print(f"    Found {len(objs)} objects")
except Exception as e:
    print(f"  ERROR: {e}")
    import traceback
    traceback.print_exc()

print("\nDone!")
