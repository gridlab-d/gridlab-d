"""
Comprehensive test of all object and property query APIs.

This test demonstrates and validates all the different ways to query
objects and their properties in GridLAB-D:

1. get_property() - Get a single property from a single object
2. get_object_properties() - Get all properties from a single object
3. get_objects_by_class() - Get names/IDs of all objects of a class
4. get_properties_by_class() - Get one property from all objects of a class
5. get_all_objects() - Get all properties from all objects of a class
"""
import os
import pytest
import gridlabd

@pytest.fixture
def gld_with_model():
    """Fixture that provides a GridLAB-D instance with the HVAC model loaded."""
    gld = gridlabd.GridLabD()
    
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    result = gld.load_glm(["gridlabd", model_path])
    assert result == gridlabd.GLDErrorCode.GLD_SUCCESS
    
    result = gld.setup_after_load()
    assert result == gridlabd.GLDErrorCode.GLD_SUCCESS
    
    return gld

class TestSingleObjectSingleProperty:
    """Test get_property() - most granular query."""
    
    def test_get_property_success(self, gld_with_model):
        """Get a single property from a known object."""
        # Get an object name first
        houses = gld_with_model.get_objects_by_class("house")
        if not houses:
            pytest.skip("No house objects in model")
        
        # Get a single property
        result, value = gld_with_model.get_property(houses[0], "floor_area")
        
        assert result == gridlabd.GLDErrorCode.GLD_SUCCESS
        assert isinstance(value, str)
        assert value != ""
    
    def test_get_property_invalid_object(self, gld_with_model):
        """Get property from non-existent object."""
        result, value = gld_with_model.get_property("nonexistent_obj", "some_prop")
        assert result == gridlabd.GLDErrorCode.GLD_OPERATION_FAILED

class TestSingleObjectAllProperties:
    """Test get_object_properties() - all properties from one object."""
    
    def test_get_object_properties_by_name(self, gld_with_model):
        """Get all properties from an object by name."""
        houses = gld_with_model.get_objects_by_class("house")
        if not houses:
            pytest.skip("No house objects in model")
        
        props = gld_with_model.get_object_properties(houses[0])
        
        # Should return a dictionary
        assert isinstance(props, dict)
        
        # Should have metadata fields
        assert "__class__" in props
        assert "__id__" in props
        assert props["__class__"] == "house"
        
        # Should have actual properties (beyond metadata)
        meta_keys = {"__class__", "__id__", "__name__"}
        property_keys = [k for k in props.keys() if k not in meta_keys]
        assert len(property_keys) > 0
        
        # All values should be strings
        for value in props.values():
            assert isinstance(value, str)
    
    def test_get_object_properties_by_id(self, gld_with_model):
        """Get all properties from an object by ID."""
        houses = gld_with_model.get_objects_by_class("house")
        if not houses:
            pytest.skip("No house objects in model")
        
        # Get by name first to find ID
        props_by_name = gld_with_model.get_object_properties(houses[0])
        obj_id = props_by_name["__id__"]
        
        # Now query by ID
        props_by_id = gld_with_model.get_object_properties(obj_id)
        
        # Should get the same object
        assert props_by_id["__id__"] == obj_id
        assert props_by_id["__class__"] == "house"

class TestAllObjectsNames:
    """Test get_objects_by_class() - just names/IDs."""
    
    def test_get_objects_by_class(self, gld_with_model):
        """Get list of object names for a class."""
        houses = gld_with_model.get_objects_by_class("house")
        
        # Should return a list
        assert isinstance(houses, list)
        
        # If there are houses, they should be strings
        if houses:
            assert all(isinstance(name, str) for name in houses)
            assert all(name != "" for name in houses)
    
    def test_get_objects_by_class_empty(self, gld_with_model):
        """Get objects for a class with no instances."""
        objs = gld_with_model.get_objects_by_class("nonexistent_class")
        assert isinstance(objs, list)
        assert len(objs) == 0

class TestAllObjectsSingleProperty:
    """Test get_properties_by_class() - one property from all objects."""
    
    def test_get_properties_by_class(self, gld_with_model):
        """Get one property from all objects of a class."""
        props = gld_with_model.get_properties_by_class("house", "floor_area")
        
        # Should return a dictionary mapping object names to values
        assert isinstance(props, dict)
        
        # If there are results, validate structure
        if props:
            for obj_name, value in props.items():
                assert isinstance(obj_name, str)
                assert isinstance(value, str)
                assert obj_name != ""
    
    def test_get_properties_by_class_count_matches(self, gld_with_model):
        """Verify count matches get_objects_by_class."""
        obj_names = gld_with_model.get_objects_by_class("house")
        props = gld_with_model.get_properties_by_class("house", "floor_area")
        
        # Should have same number of results
        assert len(props) == len(obj_names)

class TestAllObjectsAllProperties:
    """Test get_all_objects() - all properties from all objects (NEW API)."""
    
    def test_get_all_objects_structure(self, gld_with_model):
        """Get all objects with all their properties."""
        objects = gld_with_model.get_all_objects("house")
        
        # Should return a list
        assert isinstance(objects, list)
        
        if objects:
            # Each item should be a dictionary
            for obj in objects:
                assert isinstance(obj, dict)
                
                # Should have metadata
                assert "__class__" in obj
                assert "__id__" in obj
                assert obj["__class__"] == "house"
                
                # Should have properties
                meta_keys = {"__class__", "__id__", "__name__"}
                property_keys = [k for k in obj.keys() if k not in meta_keys]
                assert len(property_keys) > 0
    
    def test_get_all_objects_matches_individual_queries(self, gld_with_model):
        """Verify get_all_objects matches querying objects individually."""
        # Get all objects at once
        all_objects = gld_with_model.get_all_objects("house")
        
        # Get object names
        obj_names = gld_with_model.get_objects_by_class("house")
        
        # Count should match
        assert len(all_objects) == len(obj_names)
        
        if all_objects and obj_names:
            # Get first object both ways
            obj_from_all = all_objects[0]
            obj_from_individual = gld_with_model.get_object_properties(obj_names[0])
            
            # Should have same structure
            assert obj_from_all["__class__"] == obj_from_individual["__class__"]
            assert set(obj_from_all.keys()) == set(obj_from_individual.keys())

class TestAPIComparison:
    """Compare different APIs to show their relationships."""
    
    def test_api_comparison_workflow(self, gld_with_model):
        """Demonstrate the relationship between all APIs."""
        test_class = "house"
        
        # 1. Get object names
        obj_names = gld_with_model.get_objects_by_class(test_class)
        if not obj_names:
            pytest.skip(f"No {test_class} objects in model")
        
        # 2. Get one property from all objects
        floor_areas = gld_with_model.get_properties_by_class(test_class, "floor_area")
        assert len(floor_areas) == len(obj_names)
        
        # 3. Get all properties from one object
        first_obj_props = gld_with_model.get_object_properties(obj_names[0])
        assert first_obj_props["__class__"] == test_class
        
        # 4. Get single property from one object
        result, floor_area = gld_with_model.get_property(obj_names[0], "floor_area")
        assert result == gridlabd.GLDErrorCode.GLD_SUCCESS
        assert floor_area == first_obj_props["floor_area"]
        
        # 5. Get all objects with all properties (NEW!)
        all_objects = gld_with_model.get_all_objects(test_class)
        assert len(all_objects) == len(obj_names)
        
        # Verify consistency
        first_from_all = all_objects[0]
        assert set(first_from_all.keys()) == set(first_obj_props.keys())
    
    def test_efficiency_comparison(self, gld_with_model):
        """Show that get_all_objects is more efficient than manual loops."""
        test_class = "house"
        
        # OLD WAY: Multiple API calls
        obj_names = gld_with_model.get_objects_by_class(test_class)
        if not obj_names:
            pytest.skip(f"No {test_class} objects")
        
        old_way_results = []
        for name in obj_names:
            props = gld_with_model.get_object_properties(name)
            old_way_results.append(props)
        
        # NEW WAY: Single API call
        new_way_results = gld_with_model.get_all_objects(test_class)
        
        # Should get same number of objects
        assert len(old_way_results) == len(new_way_results)
        
        # Should have same structure
        if old_way_results and new_way_results:
            assert set(old_way_results[0].keys()) == set(new_way_results[0].keys())

class TestEdgeCases:
    """Test edge cases and error conditions."""
    
    def test_empty_class(self, gld_with_model):
        """All APIs handle classes with no objects gracefully."""
        fake_class = "nonexistent_class_xyz"
        
        # get_objects_by_class
        names = gld_with_model.get_objects_by_class(fake_class)
        assert names == []
        
        # get_properties_by_class
        props = gld_with_model.get_properties_by_class(fake_class, "some_prop")
        assert props == {}
        
        # get_all_objects
        objs = gld_with_model.get_all_objects(fake_class)
        assert objs == []
    
    def test_all_apis_return_correct_types(self, gld_with_model):
        """Verify return types are consistent."""
        test_class = "house"
        
        # get_objects_by_class -> list of strings
        names = gld_with_model.get_objects_by_class(test_class)
        assert isinstance(names, list)
        
        # get_properties_by_class -> dict of strings
        props = gld_with_model.get_properties_by_class(test_class, "floor_area")
        assert isinstance(props, dict)
        
        # get_object_properties -> dict of strings
        if names:
            obj_props = gld_with_model.get_object_properties(names[0])
            assert isinstance(obj_props, dict)
        
        # get_all_objects -> list of dicts
        all_objs = gld_with_model.get_all_objects(test_class)
        assert isinstance(all_objs, list)
        if all_objs:
            assert isinstance(all_objs[0], dict)
