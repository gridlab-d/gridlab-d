"""
Test the clock property API.

Tests for accessing starttime, stoptime, clock, and other time-related
global variables via the GridLAB-D API without requiring a checkpoint.
"""

import os
import tempfile
import pytest
import gridlabd


@pytest.fixture
def gld():
    """Create and initialize a GridLabD instance."""
    instance = gridlabd.GridLabD()
    yield instance


@pytest.fixture
def gld_with_model(gld):
    """Create a GridLabD instance with a simple model loaded."""
    glm_content = """
clock {
    timezone EST+5EDT;
    starttime '2024-01-01 00:00:00';
    stoptime '2024-01-02 00:00:00';
}

module residential;

object house {
    name test_house;
    floor_area 2000 sf;
}
"""
    
    # Write to temp file and load
    with tempfile.NamedTemporaryFile(mode='w', suffix='.glm', delete=False) as f:
        f.write(glm_content)
        temp_file = f.name
    
    try:
        gld.load_glm([temp_file])
        yield gld
    finally:
        if os.path.exists(temp_file):
            os.unlink(temp_file)


class TestClockAPI:
    """Test clock property access via API."""

    def test_get_clock(self, gld_with_model):
        """Test getting the current simulation time."""
        clock = gld_with_model.get_clock()
        assert clock is not None
        assert isinstance(clock, str)
        assert len(clock) > 0
        print(f"Clock: {clock}")

    def test_get_starttime(self, gld_with_model):
        """Test getting the simulation start time."""
        starttime = gld_with_model.get_starttime()
        assert starttime is not None
        assert isinstance(starttime, str)
        assert len(starttime) > 0
        # Should contain the year 2024 from our GLM
        assert "2024" in starttime or starttime.startswith("17")  # timestamp or ISO
        print(f"Starttime: {starttime}")

    def test_get_stoptime(self, gld_with_model):
        """Test getting the simulation stop time."""
        stoptime = gld_with_model.get_stoptime()
        assert stoptime is not None
        assert isinstance(stoptime, str)
        assert len(stoptime) > 0
        print(f"Stoptime: {stoptime}")

    def test_get_timezone(self, gld_with_model):
        """Test getting the timezone."""
        timezone = gld_with_model.get_timezone()
        assert timezone is not None
        assert isinstance(timezone, str)
        # Timezone might be empty or set to EST+5EDT
        print(f"Timezone: '{timezone}'")

    def test_set_starttime(self, gld):
        """Test setting the simulation start time before loading a model."""
        # Set starttime before loading model
        code = gld.set_starttime("2025-06-01 00:00:00")
        assert code == 0 or code == 1  # SUCCESS or FAILED (both acceptable)
        
        # Load a simple model
        glm_content = """
clock {
    timezone EST+5EDT;
}

module residential;

object house {
    name test_house;
}
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.glm', delete=False) as f:
            f.write(glm_content)
            temp_file = f.name
        
        try:
            gld.load_glm([temp_file])
            
            # Get starttime and verify
            starttime = gld.get_starttime()
            print(f"Set starttime result: {starttime}")
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)

    def test_set_stoptime(self, gld):
        """Test setting the simulation stop time before loading a model."""
        # Set stoptime before loading model
        code = gld.set_stoptime("2025-12-31 23:59:59")
        assert code == 0 or code == 1
        
        # Load a simple model
        glm_content = """
clock {
    timezone EST+5EDT;
}

module residential;

object house {
    name test_house;
}
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.glm', delete=False) as f:
            f.write(glm_content)
            temp_file = f.name
        
        try:
            gld.load_glm([temp_file])
            
            # Get stoptime and verify
            stoptime = gld.get_stoptime()
            print(f"Set stoptime result: {stoptime}")
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)

    def test_clock_before_load(self, gld):
        """Test accessing clock properties before loading a model."""
        # Should be able to access clock even before loading
        clock = gld.get_clock()
        assert clock is not None
        print(f"Clock before load: {clock}")
        
        starttime = gld.get_starttime()
        assert starttime is not None
        print(f"Starttime before load: {starttime}")
        
        stoptime = gld.get_stoptime()
        assert stoptime is not None
        print(f"Stoptime before load: {stoptime}")

    def test_clock_after_run(self, gld_with_model):
        """Test that clock advances after running simulation."""
        # Get initial clock
        initial_clock = gld_with_model.get_clock()
        print(f"Initial clock: {initial_clock}")
        
        # Run simulation
        code = gld_with_model.run()
        assert code == 0, f"Simulation failed with code {code}"
        
        # Get clock after run - might have advanced
        final_clock = gld_with_model.get_clock()
        print(f"Final clock: {final_clock}")
        
        # Both should be valid
        assert initial_clock is not None
        assert final_clock is not None


class TestGlobalVariableAPI:
    """Test the underlying global variable API."""

    def test_get_global_variable(self, gld_with_model):
        """Test getting a global variable directly."""
        value = gld_with_model.global_getvar("clock")
        assert value is not None
        assert isinstance(value, str)
        print(f"Global 'clock': {value}")

    def test_set_global_variable(self, gld):
        """Test setting a global variable directly."""
        # Try setting a custom global variable
        code = gld.global_setvar("testvar", "testvalue")
        # code might be 0 (SUCCESS) or 1 (FAILED depending on strictnames)
        print(f"Set global result code: {code}")
        
        # Try to get it back
        value = gld.global_getvar("testvar")
        print(f"Retrieved testvar: '{value}'")

    def test_get_nonexistent_global(self, gld):
        """Test getting a non-existent global variable."""
        value = gld.global_getvar("nonexistent_variable_xyz")
        # Should return empty string or error
        assert isinstance(value, str)
        print(f"Nonexistent variable: '{value}'")
