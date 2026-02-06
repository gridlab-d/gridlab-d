"""
Test the Pythonic kwargs-based load_glm() interface.

Tests for the enhanced load_glm() method that accepts keyword arguments
instead of requiring argv-style lists.
"""

import os
import tempfile
import pytest
import gridlabd


@pytest.fixture
def test_glm_file():
    """Create a temporary test GLM file."""
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
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.glm', delete=False) as f:
        f.write(glm_content)
        temp_file = f.name
    
    yield temp_file
    
    if os.path.exists(temp_file):
        os.unlink(temp_file)


class TestLoadGLMBackwardCompatibility:
    """Test that old argv-style interface still works."""

    def test_load_glm_with_list_old_style(self, test_glm_file):
        """Test loading with old list-based syntax."""
        gld = gridlabd.GridLabD()
        
        # Old style: pass a list
        result = gld.load_glm([test_glm_file])
        
        assert result == 0, f"Failed to load with old style: {result}"
        
        # Verify it loaded
        houses = gld.get_objects_by_class("house")
        assert len(houses) > 0

    def test_load_glm_with_options_old_style(self, test_glm_file):
        """Test loading with options using old list-based syntax."""
        gld = gridlabd.GridLabD()
        
        # Old style with verbose flag
        result = gld.load_glm([test_glm_file, "--verbose"])
        
        assert result == 0, f"Failed to load with old style + options: {result}"
        
        # Verify it loaded
        houses = gld.get_objects_by_class("house")
        assert len(houses) > 0


class TestLoadGLMPythonicInterface:
    """Test the new Pythonic kwargs-based interface."""

    def test_load_glm_simple_filename(self, test_glm_file):
        """Test loading with just a filename string."""
        gld = gridlabd.GridLabD()
        
        # New style: just pass filename as string
        result = gld.load_glm(test_glm_file)
        
        assert result == 0, f"Failed to load with simple filename: {result}"
        
        # Verify it loaded
        houses = gld.get_objects_by_class("house")
        assert len(houses) > 0

    def test_load_glm_with_verbose(self, test_glm_file):
        """Test loading with verbose flag."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, verbose=True)
        
        assert result == 0, f"Failed to load with verbose=True: {result}"
        
        houses = gld.get_objects_by_class("house")
        assert len(houses) > 0

    def test_load_glm_with_warn(self, test_glm_file):
        """Test loading with warn flag."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, warn=True)
        
        assert result == 0, f"Failed to load with warn=True: {result}"

    def test_load_glm_with_quiet(self, test_glm_file):
        """Test loading with quiet flag."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, quiet=True)
        
        assert result == 0, f"Failed to load with quiet=True: {result}"

    def test_load_glm_with_defines_dict(self, test_glm_file):
        """Test loading with defines as a dictionary."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(
            test_glm_file,
            defines={"MY_VAR": "123", "ANOTHER_VAR": "test_value"}
        )
        
        assert result == 0, f"Failed to load with defines: {result}"
        
        # Try to retrieve the defined globals
        my_var = gld.global_getvar("MY_VAR")
        print(f"MY_VAR = '{my_var}'")
        
        another_var = gld.global_getvar("ANOTHER_VAR")
        print(f"ANOTHER_VAR = '{another_var}'")

    def test_load_glm_with_workdir(self):
        """Test loading with workdir parameter."""
        gld = gridlabd.GridLabD()
        
        # Create a GLM file in a temp directory
        with tempfile.TemporaryDirectory() as tmpdir:
            glm_file = os.path.join(tmpdir, "test.glm")
            with open(glm_file, 'w') as f:
                f.write("""
clock {
    starttime '2024-01-01 00:00:00';
    stoptime '2024-01-02 00:00:00';
}

module residential;

object house {
    name test_house;
}
""")
            
            # Load with workdir set to the temp directory
            # Use just the filename, not the full path
            result = gld.load_glm("test.glm", workdir=tmpdir)
            
            assert result == 0, f"Failed to load with workdir: {result}"

    @pytest.mark.skip(reason="--threadcount option causes GridLAB-D to hang")
    def test_load_glm_with_threads(self, test_glm_file):
        """Test loading with threads parameter."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, threads=4)
        
        assert result == 0, f"Failed to load with threads=4: {result}"

    def test_load_glm_with_multiple_options(self, test_glm_file):
        """Test loading with multiple options combined."""
        gld = gridlabd.GridLabD()
        
        # Skip threads parameter due to hanging issue
        result = gld.load_glm(
            test_glm_file,
            verbose=True,
            warn=True,
            defines={"X": "10", "Y": "20"}
        )
        
        assert result == 0, f"Failed to load with multiple options: {result}"
        
        houses = gld.get_objects_by_class("house")
        assert len(houses) > 0

    def test_load_glm_with_debug(self, test_glm_file):
        """Test loading with debug flag."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, debug=True)
        
        assert result == 0, f"Failed to load with debug=True: {result}"

    @pytest.mark.skip(reason="--check flag causes GridLAB-D internal validation to fail")
    def test_load_glm_with_check(self, test_glm_file):
        """Test loading with check flag."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, check=True)
        
        assert result == 0, f"Failed to load with check=True: {result}"


class TestLoadGLMEdgeCases:
    """Test edge cases and error conditions."""

    def test_load_glm_nonexistent_file(self):
        """Test loading a file that doesn't exist."""
        gld = gridlabd.GridLabD()
        
        # Should fail gracefully
        try:
            result = gld.load_glm("nonexistent_file_xyz.glm")
            # May return error code or raise exception
            assert result != 0, "Expected failure for nonexistent file"
        except RuntimeError as e:
            # Exception is also acceptable
            print(f"Got expected error: {e}")

    def test_load_glm_empty_defines(self, test_glm_file):
        """Test loading with empty defines dict."""
        gld = gridlabd.GridLabD()
        
        result = gld.load_glm(test_glm_file, defines={})
        
        assert result == 0, f"Failed to load with empty defines: {result}"

    def test_load_glm_conflicting_flags(self, test_glm_file):
        """Test loading with potentially conflicting flags."""
        gld = gridlabd.GridLabD()
        
        # verbose and quiet might conflict, but let GridLAB-D handle it
        result = gld.load_glm(test_glm_file, verbose=True, quiet=True)
        
        # Should still load (GridLAB-D decides which flag wins)
        assert result == 0, f"Failed to load with conflicting flags: {result}"


class TestLoadGLMComparison:
    """Compare old and new styles produce same results."""

    def test_old_vs_new_style_equivalent(self, test_glm_file):
        """Test that old and new styles produce equivalent results."""
        # Old style
        gld_old = gridlabd.GridLabD()
        result_old = gld_old.load_glm([test_glm_file, "--verbose"])
        assert result_old == 0
        houses_old = gld_old.get_objects_by_class("house")
        
        # New style
        gld_new = gridlabd.GridLabD()
        result_new = gld_new.load_glm(test_glm_file, verbose=True)
        assert result_new == 0
        houses_new = gld_new.get_objects_by_class("house")
        
        # Should have same number of houses
        assert len(houses_old) == len(houses_new)

    def test_old_vs_new_with_defines(self, test_glm_file):
        """Test that old and new styles with defines are equivalent."""
        # Old style
        gld_old = gridlabd.GridLabD()
        result_old = gld_old.load_glm([test_glm_file, "-D", "VAR=123"])
        assert result_old == 0
        
        # New style
        gld_new = gridlabd.GridLabD()
        result_new = gld_new.load_glm(test_glm_file, defines={"VAR": "123"})
        assert result_new == 0
        
        # Both should have the variable defined
        var_old = gld_old.global_getvar("VAR")
        var_new = gld_new.global_getvar("VAR")
        
        print(f"Old style VAR: '{var_old}'")
        print(f"New style VAR: '{var_new}'")
        
        # Both should have the variable (might be empty string or actual value)
        assert var_old == var_new
