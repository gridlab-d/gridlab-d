"""
Test the new get_all_objects() API that returns all objects of a class
with their properties as a list of dictionaries.
"""

import os
import pytest
import gridlabd


@pytest.fixture
def gld_with_model():
    """Fixture that provides a GridLAB-D instance with the HVAC model loaded."""
    gld = gridlabd.GridLabD()
    
    # Load test model
    model_path = os.path.join(os.path.dirname(__file__), "test_HVAC_balance.glm")
    result = gld.load_glm(["gridlabd", model_path])
    assert result == gridlabd.GLDErrorCode.SUCCESS, "Failed to load model"
    
    # Initialize
    result = gld.setup_after_load()
    assert result == gridlabd.GLDErrorCode.SUCCESS, "Failed to initialize model"
    
    return gld


def test_get_all_objects_returns_list(gld_with_model):
    """Test that get_all_objects returns a list."""
    objects = gld_with_model.get_all_objects("house")
    print(objects)
    assert isinstance(objects, list), "get_all_objects should return a list"


def test_get_all_objects_structure(gld_with_model):
    """Test that each object has the expected structure with metadata fields."""
    classes = gld_with_model.get_all_classes()
    
    # Find a class that exists in the model
    test_class = "house" if "house" in classes else classes[0] if classes else None
    assert test_class is not None, "No classes found in model"
    
    objects = gld_with_model.get_all_objects(test_class)
    
    if len(objects) > 0:
        first_obj = objects[0]
        
        # Check required metadata fields
        assert "__class__" in first_obj, "Missing __class__ field"
        assert "__id__" in first_obj, "Missing __id__ field"
        assert first_obj["__class__"] == test_class, f"__class__ mismatch: expected {test_class}"
        
        # __name__ is optional (objects may be unnamed)
        # Just verify it's either present or absent
        if "__name__" in first_obj:
            assert isinstance(first_obj["__name__"], str), "__name__ should be a string"


def test_get_all_objects_has_properties(gld_with_model):
    """Test that objects include their properties beyond metadata."""
    classes = gld_with_model.get_all_classes()
    test_class = "house" if "house" in classes else classes[0] if classes else None
    assert test_class is not None
    
    objects = gld_with_model.get_all_objects(test_class)
    
    if len(objects) > 0:
        first_obj = objects[0]
        meta_keys = {"__class__", "__id__", "__name__"}
        property_keys = [k for k in first_obj.keys() if k not in meta_keys]
        
        # Should have at least some properties
        assert len(property_keys) > 0, "Object should have properties beyond metadata"
        
        # All values should be strings
        for key in property_keys:
            assert isinstance(first_obj[key], str), f"Property {key} should be a string"


def test_get_all_objects_nonexistent_class(gld_with_model):
    """Test that requesting a non-existent class returns an empty list."""
    objects = gld_with_model.get_all_objects("nonexistent_class_xyz")
    assert isinstance(objects, list), "Should return a list even for non-existent class"
    assert len(objects) == 0, "Should return empty list for non-existent class"


def test_get_all_objects_count_matches_class(gld_with_model):
    """Test that get_all_objects count matches get_objects_by_class count."""
    classes = gld_with_model.get_all_classes()
    
    if classes:
        test_class = classes[0]
        
        # Get object names via the old API
        object_names = gld_with_model.get_objects_by_class(test_class)
        
        # Get full objects via the new API
        objects = gld_with_model.get_all_objects(test_class)
        
        # Counts should match
        assert len(objects) == len(object_names), \
            f"get_all_objects count ({len(objects)}) should match get_objects_by_class count ({len(object_names)})"


def test_get_all_objects_multiple_classes(gld_with_model):
    """Test that we can retrieve objects from multiple classes."""
    classes = gld_with_model.get_all_classes()
    
    # Try to get objects from the first 3 classes
    for test_class in classes[:3]:
        objects = gld_with_model.get_all_objects(test_class)
        
        # Should always return a list
        assert isinstance(objects, list), f"Should return list for class {test_class}"
        
        # If objects exist, validate structure
        if len(objects) > 0:
            assert "__class__" in objects[0], f"Missing __class__ in {test_class} object"
            assert objects[0]["__class__"] == test_class, f"Class mismatch for {test_class}"
