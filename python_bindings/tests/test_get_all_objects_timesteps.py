"""
Simple test: Query all objects at START, MIDDLE, and END of simulation.

Demonstrates get_all_objects() captures changing state during simulation.
"""

import pytest
import gridlabd
from pathlib import Path


@pytest.fixture
def gld_setup():
    """Setup GridLabD instance with test model loaded"""
    gld = gridlabd.GridLabD()
    test_dir = Path(__file__).parent
    gld.set_working_directory(str(test_dir))
    
    # Load model
    result = gld.load_glm(["gridlabd", "./test_HVAC_balance.glm"])
    assert result == gridlabd.GLDErrorCode.SUCCESS, "Load failed"
    
    result = gld.setup_after_load()
    assert result == gridlabd.GLDErrorCode.SUCCESS, "Setup failed"
    
    yield gld


def test_get_all_objects_at_different_timesteps(gld_setup):
    """Test that get_all_objects() captures changing state during simulation"""
    gld = gld_setup
    
    print("\n" + "="*70)
    print("TEST: get_all_objects() at START, MIDDLE, END")
    print("="*70)
    
    # START: Get all houses before simulation
    print("\n[START] Getting all houses BEFORE simulation...")
    start_houses = gld.get_all_objects("house")
    
    assert start_houses, "No houses found in model"
    assert len(start_houses) > 0, "Expected at least one house"
    
    print(f"Found {len(start_houses)} house(s)")
    for house in start_houses:
        name = house.get('__name__', house['__id__'])
        temp = house.get('air_temperature', 'N/A')
        power = house.get('hvac_power', 'N/A')
        print(f"  {name}: temp={temp}, power={power}")
    
    # Run to MIDDLE (10 steps)
    print("\n[RUNNING] Stepping to middle...")
    for i in range(10):
        result, sim_time = gld.step()
        if result != gridlabd.GLDErrorCode.SUCCESS:
            break
    
    # MIDDLE: Get all houses mid-simulation
    print("\n[MIDDLE] Getting all houses DURING simulation (after 10 steps)...")
    middle_houses = gld.get_all_objects("house")
    
    assert len(middle_houses) == len(start_houses), "Number of houses should remain constant"
    
    print(f"Found {len(middle_houses)} house(s)")
    for house in middle_houses:
        name = house.get('__name__', house['__id__'])
        temp = house.get('air_temperature', 'N/A')
        power = house.get('hvac_power', 'N/A')
        print(f"  {name}: temp={temp}, power={power}")
    
    # Run to END (90 more steps = 100 total)
    print("\n[RUNNING] Stepping to end...")
    for i in range(90):
        result, sim_time = gld.step()
        if result != gridlabd.GLDErrorCode.SUCCESS:
            # Simulation ended or error occurred
            break
    
    # END: Get all houses after simulation
    print("\n[END] Getting all houses AFTER simulation (after 100 steps)...")
    end_houses = gld.get_all_objects("house")
    
    assert len(end_houses) == len(start_houses), "Number of houses should remain constant"
    
    print(f"Found {len(end_houses)} house(s)")
    for house in end_houses:
        name = house.get('__name__', house['__id__'])
        temp = house.get('air_temperature', 'N/A')
        power = house.get('hvac_power', 'N/A')
        print(f"  {name}: temp={temp}, power={power}")
    
    # COMPARISON & VALIDATION
    print("\n" + "="*70)
    print("COMPARISON")
    print("="*70)
    
    house_id = start_houses[0]['__id__']
    name = start_houses[0].get('__name__', house_id)
    
    start_temp = start_houses[0].get('air_temperature')
    middle_temp = middle_houses[0].get('air_temperature')
    end_temp = end_houses[0].get('air_temperature')
    
    # Verify we got valid temperature values
    assert start_temp is not None, "Start temperature should not be None"
    assert middle_temp is not None, "Middle temperature should not be None"
    assert end_temp is not None, "End temperature should not be None"
    
    print(f"\nHouse '{name}' temperature over time:")
    print(f"  START:  {start_temp}")
    print(f"  MIDDLE: {middle_temp}")
    print(f"  END:    {end_temp}")
    
    # At least one temperature should be different (simulation should change state)
    temps = [start_temp, middle_temp, end_temp]
    assert len(set(temps)) > 1, "Temperature should change during simulation"
    
    print("\n✓ Successfully captured state at 3 different timesteps!")
    print("✓ get_all_objects() returns current simulation state")
    print("="*70)
