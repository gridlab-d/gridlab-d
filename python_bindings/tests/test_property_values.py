"""
Test get_object_property_value() and get_object_properties_detailed().

These tests cover issue #1563: Need values-only way of getting object properties.

Tests verify that:
1. get_object_property_value() returns native Python types (float, int, complex, bool, str)
   without units appended.
2. get_object_properties_detailed() returns per-property metadata dicts with
   typed values, units, data type names, and access information.
"""

from pathlib import Path

import pytest
import gridlabd


@pytest.fixture
def gld_with_house():
    """Create a GridLabD instance with a simple house model."""
    gld = gridlabd.GridLabD()
    gld.set_working_directory(str(Path(__file__).parent))

    # Load the HVAC test model which has houses and meters
    gld.load_glm(["test_HVAC_balance.glm"])

    yield gld


# ---------------------------------------------------------------------------
# get_object_property_value()
# ---------------------------------------------------------------------------

class TestGetObjectPropertyValue:
    """Test get_object_property_value() returns native Python types."""

    def test_double_returns_float(self, gld_with_house):
        """A PT_double property should come back as a Python float."""
        houses = gld_with_house.get_objects_by_class("house")
        assert len(houses) > 0, "No houses found in model"
        house = houses[0]

        value = gld_with_house.get_object_property_value(house, "floor_area")
        assert isinstance(value, float), f"Expected float, got {type(value)}: {value}"
        print(f"{house}.floor_area = {value} (type={type(value).__name__})")

    def test_complex_returns_complex(self, gld_with_house):
        """A PT_complex property should come back as a Python complex."""
        meters = gld_with_house.get_objects_by_class("triplex_meter")
        if not meters:
            pytest.skip("No triplex_meter objects in model")
        meter = meters[0]

        value = gld_with_house.get_object_property_value(meter, "measured_power")
        assert isinstance(value, complex), f"Expected complex, got {type(value)}: {value}"
        print(f"{meter}.measured_power = {value} (type={type(value).__name__})")

    def test_enumeration_returns_str(self, gld_with_house):
        """A PT_enumeration property should come back as a Python str."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        value = gld_with_house.get_object_property_value(house, "system_mode")
        assert isinstance(value, str), f"Expected str, got {type(value)}: {value}"
        print(f"{house}.system_mode = {value!r}")

    def test_no_unit_suffix(self, gld_with_house):
        """The returned value must NOT contain a unit suffix."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        value = gld_with_house.get_object_property_value(house, "floor_area")
        # Must be a bare float, not a string like "2500 sf"
        assert isinstance(value, (int, float)), f"Expected numeric, got {type(value)}: {value}"

    def test_nonexistent_property_raises(self, gld_with_house):
        """Querying a non-existent property should raise RuntimeError."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        with pytest.raises(RuntimeError):
            gld_with_house.get_object_property_value(house, "nonexistent_xyz")

    def test_nonexistent_object_raises(self, gld_with_house):
        """Querying a non-existent object should raise RuntimeError."""
        with pytest.raises(RuntimeError):
            gld_with_house.get_object_property_value("no_such_object_999", "floor_area")

    def test_value_matches_get_property(self, gld_with_house):
        """The typed value should be numerically equal to the raw string value."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        # Get raw string
        code, raw = gld_with_house.get_property(house, "floor_area", typed=False)
        assert code == 0

        # Get typed value
        typed = gld_with_house.get_object_property_value(house, "floor_area")

        # Extract numeric part from raw string (e.g. "2500 sf" → 2500.0)
        raw_numeric = float(raw.split()[0])
        assert abs(typed - raw_numeric) < 1e-6, (
            f"Typed value {typed} doesn't match raw '{raw}' (parsed {raw_numeric})"
        )


# ---------------------------------------------------------------------------
# get_object_properties_detailed()
# ---------------------------------------------------------------------------

class TestGetObjectPropertiesDetailed:
    """Test get_object_properties_detailed() returns rich metadata."""

    def test_returns_dict_of_dicts(self, gld_with_house):
        """Result should be dict[str, dict]."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        props = gld_with_house.get_object_properties_detailed(house)
        assert isinstance(props, dict), f"Expected dict, got {type(props)}"
        assert len(props) > 0, "Expected at least one property"

        # Each value should be a metadata dict
        for prop_name, meta in list(props.items())[:3]:
            assert isinstance(meta, dict), f"{prop_name}: expected dict, got {type(meta)}"
            print(f"  {prop_name}: {meta}")

    def test_metadata_keys(self, gld_with_house):
        """Each property metadata dict should have the required keys."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        props = gld_with_house.get_object_properties_detailed(house)
        required_keys = {"value", "unit", "type", "access", "description"}

        for prop_name, meta in props.items():
            missing = required_keys - set(meta.keys())
            assert not missing, f"{prop_name} is missing keys: {missing}"

    def test_value_has_correct_type(self, gld_with_house):
        """Typed values should be Python-native, not raw strings with units."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        props = gld_with_house.get_object_properties_detailed(house)

        # floor_area should be a float
        if "floor_area" in props:
            fa = props["floor_area"]
            assert isinstance(fa["value"], float), (
                f"floor_area value should be float, got {type(fa['value'])}: {fa['value']}"
            )
            assert fa["unit"] != "", "floor_area should have a unit"
            print(f"floor_area: value={fa['value']}, unit={fa['unit']}, "
                  f"type={fa['type']}, access={fa['access']}")

    def test_access_field(self, gld_with_house):
        """Properties should report 'read-only' or 'read-write' access."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        props = gld_with_house.get_object_properties_detailed(house)
        valid_access = {"read-only", "read-write"}

        for prop_name, meta in props.items():
            assert meta["access"] in valid_access, (
                f"{prop_name}: unexpected access value '{meta['access']}'"
            )

    def test_no_internal_keys(self, gld_with_house):
        """Internal metadata keys like __class__ should be excluded."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        props = gld_with_house.get_object_properties_detailed(house)
        for key in props:
            assert not (key.startswith("__") and key.endswith("__")), (
                f"Internal key '{key}' should not appear in detailed properties"
            )

    def test_complex_property_metadata(self, gld_with_house):
        """Complex-valued properties should have complex Python values."""
        meters = gld_with_house.get_objects_by_class("triplex_meter")
        if not meters:
            pytest.skip("No triplex_meter objects in model")
        meter = meters[0]

        props = gld_with_house.get_object_properties_detailed(meter)
        if "measured_power" in props:
            mp = props["measured_power"]
            assert isinstance(mp["value"], complex), (
                f"measured_power should be complex, got {type(mp['value'])}: {mp['value']}"
            )
            assert mp["type"] == "complex", f"Expected type 'complex', got '{mp['type']}'"

            # Access mode can vary by module/build; verify the string matches
            # property-info access flags rather than assuming a fixed mode.
            valid_access = {"read-only", "read-write"}
            assert mp["access"] in valid_access, (
                f"measured_power has invalid access '{mp['access']}'"
            )

            info_code, info = gld_with_house.get_property_info(meter, "measured_power")
            if info_code == 0:
                pa_w = 0x02  # write access bit from PROPERTYACCESS
                expected_access = "read-write" if (int(info.get("access", 0x0F)) & pa_w) else "read-only"
                assert mp["access"] == expected_access, (
                    f"measured_power access mismatch: detailed='{mp['access']}', expected='{expected_access}'"
                )
            print(f"measured_power: {mp}")

    def test_nonexistent_object_raises(self, gld_with_house):
        """Querying a non-existent object should raise RuntimeError."""
        with pytest.raises(RuntimeError):
            gld_with_house.get_object_properties_detailed("no_such_object_999")


# ---------------------------------------------------------------------------
# Consistency checks
# ---------------------------------------------------------------------------

class TestConsistency:
    """Verify that the new APIs are consistent with each other."""

    def test_value_matches_detailed(self, gld_with_house):
        """get_object_property_value() should match the 'value' field
        from get_object_properties_detailed()."""
        houses = gld_with_house.get_objects_by_class("house")
        house = houses[0]

        single_val = gld_with_house.get_object_property_value(house, "floor_area")
        detailed = gld_with_house.get_object_properties_detailed(house)

        assert "floor_area" in detailed
        assert single_val == detailed["floor_area"]["value"], (
            f"Single value {single_val} != detailed value {detailed['floor_area']['value']}"
        )


if __name__ == "__main__":
    pytest.main([__file__, "-v", "-s"])
