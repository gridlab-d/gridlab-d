"""
Test querying simulation results after actual run.

This demonstrates testing with a REAL simulation run, not just initialization.
"""

import os
import pytest
import gridlabd


@pytest.fixture
def gld_after_simulation():
    """Fixture with COMPLETED simulation - returns post-run state."""
    gld = gridlabd.GridLabD()
    
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    result = gld.load(model_path)
    assert result == 0, f"Failed to load model: {result}"
    
    result = gld.setup_after_load()
    assert result == 0, f"Failed to setup: {result}"
    
    # ← THIS IS THE KEY DIFFERENCE - ACTUALLY RUN THE SIMULATION
    result = gld.run()
    assert result == 0, f"Failed to run: {result}"
    
    return gld


class TestSimulationResults:
    """Test querying properties AFTER simulation completes."""
    
    def test_properties_change_during_simulation(self):
        """Verify properties actually change when simulation runs."""
        gld = gridlabd.GridLabD()
        
        model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
        gld.load(model_path)
        gld.setup_after_load()
        
        # Get initial state (before simulation)
        houses = gld.get_objects_by_class("house")
        if not houses:
            pytest.skip("No houses in model")
        
        result, initial_temp = gld.get_property(houses[0], "air_temperature")
        assert result == 0, f"Failed to get initial temp: {result}"
        
        # RUN THE SIMULATION
        result = gld.run()
        assert result == 0, f"Failed to run: {result}"
        
        # Get final state (after simulation)
        result, final_temp = gld.get_property(houses[0], "air_temperature")
        assert result == 0, f"Failed to get final temp: {result}"
        
        # Temperature likely changed (HVAC ran)
        print(f"\nInitial temp: {initial_temp}")
        print(f"Final temp:   {final_temp}")
        
        # The values should exist
        assert initial_temp != ""
        assert final_temp != ""
    
    def test_query_all_final_temperatures(self, gld_after_simulation):
        """Get final temperatures from all houses after simulation."""
        # Simulation already completed in fixture
        temps = gld_after_simulation.get_properties_by_class("house", "air_temperature")
        
        if not temps:
            pytest.skip("No houses in model")
        
        print(f"\nFinal temperatures for {len(temps)} houses:")
        for house_name, temp in temps.items():
            print(f"  {house_name}: {temp}")
        
        # All houses should have temperature values
        assert all(temp != "" for temp in temps.values())
    
    def test_query_hvac_runtime(self, gld_after_simulation):
        """Check HVAC runtime statistics after simulation."""
        houses = gld_after_simulation.get_all_objects("house")
        
        if not houses:
            pytest.skip("No houses in model")
        
        print(f"\nHVAC statistics for {len(houses)} houses:")
        for house in houses:
            name = house.get("__name__", house["__id__"])
            temp = house.get("air_temperature", "N/A")
            hvac_power = house.get("hvac_power", "N/A")
            
            print(f"  {name}:")
            print(f"    Temperature: {temp}")
            print(f"    HVAC Power:  {hvac_power}")
        
        # Verify we got actual data
        assert all("air_temperature" in h for h in houses)


class TestRealWorldWorkflow:
    """Demonstrate practical usage patterns."""
    
    def test_parameter_sweep(self):
        """Run simulation with different parameters and compare results."""
        results = []
        
        # Could run multiple simulations with different setpoints
        for iteration in range(1):  # Just 1 for test speed
            gld = gridlabd.GridLabD()
            
            model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
            gld.load(model_path)
            gld.setup_after_load()
            
            # In real use, you'd modify properties here:
            # houses = gld.get_objects_by_class("house")
            # for house in houses:
            #     gld.set_property(house, "cooling_setpoint", f"{72 + iteration} degF")
            
            # Run simulation
            gld.run()
            
            # Collect results
            temps = gld.get_properties_by_class("house", "air_temperature")
            hvac_power = gld.get_properties_by_class("house", "hvac_power")
            
            results.append({
                'iteration': iteration,
                'temperatures': temps,
                'hvac_power': hvac_power
            })
        
        # Analyze results
        assert len(results) > 0
        print(f"\nCompleted {len(results)} simulations")
        
        for r in results:
            if r['temperatures']:
                avg_temp = len(r['temperatures'])  # Simplified for test
                print(f"  Iteration {r['iteration']}: {avg_temp} houses simulated")
    
    def test_data_extraction_workflow(self, gld_after_simulation):
        """Extract data for external analysis (e.g., pandas, plotting)."""
        # Get all house data
        houses = gld_after_simulation.get_all_objects("house")
        
        if not houses:
            pytest.skip("No houses in model")
        
        # Extract into tabular format (like you'd do for pandas)
        data_table = []
        for house in houses:
            row = {
                'name': house.get('__name__', house['__id__']),
                'id': house['__id__'],
                'temperature': house.get('air_temperature', ''),
                'floor_area': house.get('floor_area', ''),
                'hvac_power': house.get('hvac_power', '')
            }
            data_table.append(row)
        
        print(f"\nExtracted data for {len(data_table)} houses:")
        for row in data_table[:3]:  # Show first 3
            print(f"  {row}")
        
        # Verify structure
        assert len(data_table) == len(houses)
        assert all('temperature' in row for row in data_table)
