"""
Simple test to verify basic GridLAB-D loading and execution.
"""
import os
import gridlabd


def test_simple_load_and_run():
    """Test basic model loading and execution."""
    # Create GridLAB-D instance
    gld = gridlabd.GridLabD()
    
    # Load a simple model
    model_path = os.path.join(os.path.dirname(__file__), "models", "minimal.glm")
    result = gld.load(model_path)
    
    print(f"Load result: {result}")
    assert result == 0, f"Failed to load model: {result}"
    
    # Run the model
    result = gld.run()
    print(f"Run result: {result}")
    assert result == 0, f"Failed to run model: {result}"


def test_simple_load_with_argv():
    """Test loading with argv-style arguments."""
    gld = gridlabd.GridLabD()
    
    model_path = os.path.join(os.path.dirname(__file__), "models", "minimal.glm")
    result = gld.load_glm([model_path])
    
    print(f"Load result: {result}")
    assert result == 0, f"Failed to load model with load_glm: {result}"
    
    # Run the model
    result = gld.run()
    print(f"Run result: {result}")
    assert result == 0, f"Failed to run model: {result}"


if __name__ == "__main__":
    print("Running simple load test...")
    test_simple_load_and_run()
    print("✓ Test passed!")
    
    print("\nRunning simple load with argv test...")
    test_simple_load_with_argv()
    print("✓ Test passed!")
