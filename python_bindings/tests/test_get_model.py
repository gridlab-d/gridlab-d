"""
Test the get_model() API that returns the entire model as a nested structure.
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
    assert result == gridlabd.GLDErrorCode.SUCCESS
    
    result = gld.setup_after_load()
    assert result == gridlabd.GLDErrorCode.SUCCESS
    
    return gld


def test_get_model_returns_dict(gld_with_model):
    """Test that get_model returns a dictionary."""
    model = gld_with_model.get_model()
    assert isinstance(model, dict), "get_model should return a dictionary"


def test_get_model_structure(gld_with_model):
    """Test the structure of the returned model."""
    model = gld_with_model.get_model()
    
    # Should have at least some classes
    assert len(model) > 0, "Model should contain at least one class"
    
    # Each value should be a list
    for class_name, objects in model.items():
        assert isinstance(class_name, str), "Class names should be strings"
        assert isinstance(objects, list), f"Objects for class {class_name} should be a list"
        
        # If there are objects, each should be a dict
        if objects:
            for obj in objects:
                assert isinstance(obj, dict), f"Each object in {class_name} should be a dict"
                assert "__class__" in obj, "Object should have __class__ field"
                assert obj["__class__"] == class_name, "Object __class__ should match"


def test_get_model_matches_individual_calls(gld_with_model):
    """Verify get_model matches calling get_all_objects for each class."""
    # Get entire model
    model = gld_with_model.get_model()
    
    # Get all classes
    classes = gld_with_model.get_all_classes()
    
    # Model should only contain classes with objects
    for class_name in model.keys():
        assert class_name in classes, f"Model class {class_name} should be in classes list"
    
    # For each class in the model, verify it matches get_all_objects
    for class_name, model_objects in model.items():
        individual_objects = gld_with_model.get_all_objects(class_name)
        
        # Should have same number of objects
        assert len(model_objects) == len(individual_objects), \
            f"Class {class_name}: model has {len(model_objects)} objects, " \
            f"get_all_objects returned {len(individual_objects)}"
        
        # If there are objects, compare structure
        if model_objects and individual_objects:
            # Should have same keys
            assert set(model_objects[0].keys()) == set(individual_objects[0].keys()), \
                f"Object structure should match for {class_name}"


def test_get_model_empty_classes_excluded(gld_with_model):
    """Verify that classes with no objects are excluded from the model."""
    model = gld_with_model.get_model()
    all_classes = gld_with_model.get_all_classes()
    
    # Model should have fewer or equal classes than the total
    assert len(model) <= len(all_classes), \
        "Model should not contain more classes than exist"
    
    # Every class in the model should have at least one object
    for class_name, objects in model.items():
        assert len(objects) > 0, \
            f"Class {class_name} in model should have at least one object"


def test_get_model_object_counts(gld_with_model):
    """Test that object counts in the model are correct."""
    model = gld_with_model.get_model()
    
    # Count total objects
    total_from_model = sum(len(objects) for objects in model.values())
    
    # Count via individual queries
    classes = gld_with_model.get_all_classes()
    total_from_queries = 0
    for cls in classes:
        objs = gld_with_model.get_all_objects(cls)
        total_from_queries += len(objs)
    
    # Should match
    assert total_from_model == total_from_queries, \
        f"Total objects from get_model ({total_from_model}) should match " \
        f"sum of get_all_objects ({total_from_queries})"


def test_get_model_contains_expected_classes(gld_with_model):
    """Test that the model contains expected classes from the test file."""
    model = gld_with_model.get_model()
    
    # The HVAC test model should have houses
    # (But we check dynamically in case the test file changes)
    all_classes = gld_with_model.get_all_classes()
    
    # If house class exists and has objects, it should be in the model
    if "house" in all_classes:
        houses_from_query = gld_with_model.get_all_objects("house")
        if houses_from_query:
            assert "house" in model, "Model should contain 'house' class"
            assert len(model["house"]) == len(houses_from_query), \
                "House count should match"


def test_get_model_vs_get_all_classes(gld_with_model):
    """Compare get_model classes with get_all_classes."""
    model = gld_with_model.get_model()
    all_classes = gld_with_model.get_all_classes()
    
    # Every class in the model should be in all_classes
    for class_name in model.keys():
        assert class_name in all_classes, \
            f"Model class {class_name} should be in all_classes"
    
    # Model should be a subset of all_classes (excluding empty classes)
    assert set(model.keys()).issubset(set(all_classes)), \
        "Model classes should be a subset of all classes"
