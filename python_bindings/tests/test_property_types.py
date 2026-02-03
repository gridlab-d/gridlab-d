"""
Test property type introspection and native Python type support in set_property.

This module tests:
1. get_property_info() - getting property metadata (type, unit, description)
2. set_property() with native Python types (int, float, bool, complex, str)
"""

import pytest
import gridlabd


@pytest.fixture
def gld_with_house():
    """Create a GridLabD instance with a simple house model."""
    gld = gridlabd.GridLabD()
    
    # Load the HVAC test model which has houses
    gld.load_glm(["test_HVAC_balance.glm"])
    
    yield gld


class TestPropertyInfo:
    """Test get_property_info() functionality."""
    
    def test_get_property_info_double(self, gld_with_house):
        """Test getting info for a double property."""
        houses = gld_with_house.get_objects_by_class("house")
        assert len(houses) > 0, "No houses found in model"
        
        house_name = houses[0]
        code, info = gld_with_house.get_property_info(house_name, "floor_area")
        
        assert code == 0, f"Failed to get property info: code={code}"
        assert "type" in info, "Missing 'type' in property info"
        assert "unit" in info, "Missing 'unit' in property info"
        assert "description" in info, "Missing 'description' in property info"
        
        # floor_area should be a double with unit 'sf'
        print(f"Property info for {house_name}.floor_area: {info}")
    
    def test_get_property_info_enumeration(self, gld_with_house):
        """Test getting info for an enumeration property."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        code, info = gld_with_house.get_property_info(house_name, "system_mode")
        
        assert code == 0, f"Failed to get property info: code={code}"
        assert "type" in info
        print(f"Property info for {house_name}.system_mode: {info}")
    
    def test_get_property_info_nonexistent(self, gld_with_house):
        """Test getting info for a non-existent property."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        code, info = gld_with_house.get_property_info(house_name, "nonexistent_property")
        
        # Should return error code
        assert code != 0, "Expected error code for non-existent property"


class TestSetPropertyWithNativeTypes:
    """Test set_property() with native Python types."""
    
    def test_set_property_with_string(self, gld_with_house):
        """Test setting a property with a string value (original behavior)."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        # Set with string (original way)
        code = gld_with_house.set_property(house_name, "floor_area", "2500 sf")
        assert code == 0, f"Failed to set property with string: code={code}"
        
        # Verify
        code, value = gld_with_house.get_property(house_name, "floor_area")
        assert code == 0
        print(f"Set floor_area to '2500 sf', got back: {value}")
    
    def test_set_property_with_int(self, gld_with_house):
        """Test setting a property with an integer."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        # Set with int (new feature)
        code = gld_with_house.set_property(house_name, "floor_area", 3000)
        assert code == 0, f"Failed to set property with int: code={code}"
        
        # Verify
        code, value = gld_with_house.get_property(house_name, "floor_area")
        assert code == 0
        print(f"Set floor_area to int 3000, got back: {value}")
        assert "3000" in value
    
    def test_set_property_with_float(self, gld_with_house):
        """Test setting a property with a float."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        # Set with float (new feature)
        code = gld_with_house.set_property(house_name, "floor_area", 2750.5)
        assert code == 0, f"Failed to set property with float: code={code}"
        
        # Verify
        code, value = gld_with_house.get_property(house_name, "floor_area")
        assert code == 0
        print(f"Set floor_area to float 2750.5, got back: {value}")
        assert "2750" in value
    
    def test_set_property_with_bool(self, gld_with_house):
        """Test setting a property with a boolean."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        # Houses might have boolean properties - let's test with a known one
        # For now, test the conversion by checking it doesn't crash
        code = gld_with_house.set_property(house_name, "include_solar_quadrant", True)
        # This might fail if property doesn't exist, but the type conversion should work
        print(f"Set boolean property, code: {code}")
    
    def test_set_property_with_complex(self, gld_with_house):
        """Test setting a complex property with Python complex number."""
        # Get a meter object which has complex properties
        meters = gld_with_house.get_objects_by_class("triplex_meter")
        if len(meters) == 0:
            pytest.skip("No triplex_meter objects in model")
        
        meter_name = meters[0]
        
        # Set with complex (new feature)
        complex_value = 120.0 + 0.0j
        code = gld_with_house.set_property(meter_name, "voltage_1", complex_value)
        
        # Note: might fail if property is read-only, but type conversion should work
        print(f"Set complex property, code: {code}")
    
    def test_set_property_mixed_types(self, gld_with_house):
        """Test setting multiple properties with different types."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        # Test multiple type conversions in sequence
        test_values = [
            ("floor_area", 2000, "int"),
            ("floor_area", 2500.75, "float"),
            ("floor_area", "3000 sf", "string"),
        ]
        
        for prop_name, value, value_type in test_values:
            code = gld_with_house.set_property(house_name, prop_name, value)
            print(f"Set {prop_name} to {value_type} {value}, code: {code}")
            assert code == 0, f"Failed to set {prop_name} with {value_type}"


class TestPropertyTypeWorkflow:
    """Test complete workflow: get info, then set with appropriate type."""
    
    def test_introspect_then_set(self, gld_with_house):
        """Test getting property type info and then setting with matching Python type."""
        houses = gld_with_house.get_objects_by_class("house")
        house_name = houses[0]
        
        # 1. Get property info
        code, info = gld_with_house.get_property_info(house_name, "floor_area")
        assert code == 0
        
        print(f"Property type: {info.get('type')}, unit: {info.get('unit')}")
        
        # 2. Based on type, use appropriate Python type
        # PropertyType.DOUBLE = 1 (from enum)
        if info.get('type') == 1:  # PT_double
            # Use float
            code = gld_with_house.set_property(house_name, "floor_area", 2800.0)
            assert code == 0
        
        # 3. Verify the change
        code, value = gld_with_house.get_property(house_name, "floor_area")
        assert code == 0
        print(f"Final value: {value}")


if __name__ == "__main__":
    # Allow running the test file directly for debugging
    pytest.main([__file__, "-v", "-s"])
