# GridLAB-D Python Bindings

This package provides Python bindings for GridLAB-D, a power system simulation platform.

## Installation

1. **Build the GridLAB-D core** (if not already built):
   ```bash
   # From the repository root
   cd build
   cmake --build . --parallel
   ```

2. **Install the Python package in development mode**:
   ```bash
   cd python_bindings
   pip install -e .
   ```

3. **Run tests to verify installation**:
   ```bash
   cd python_bindings/tests
   pytest -v
   ```

## API Usage Examples

### Basic Usage

```python
import gridlabd

# Create a GridLAB-D instance
gld = gridlabd.GridLabD()

# Load a model file
result = gld.load("path/to/model.glm")
assert result == 0, "Failed to load model"

# Initialize the model
result = gld.setup_after_load()
assert result == 0, "Failed to initialize"

# Run the simulation
result = gld.run()
assert result == 0, "Simulation failed"

print("Simulation completed successfully!")
```

### Querying Objects and Properties

```python
import gridlabd

# Load and initialize model
gld = gridlabd.GridLabD()
gld.load("test_HVAC_balance.glm")
gld.setup_after_load()

# Get all classes in the model
classes = gld.get_all_classes()
print(f"Classes in model: {classes}")

# Get all objects of a specific class
houses = gld.get_objects_by_class("house")
print(f"Found {len(houses)} houses")

# Get properties from a single object
if houses:
    props = gld.get_object_properties(houses[0])
    print(f"Floor area: {props.get('floor_area')}")

# Get all objects with all their properties
all_houses = gld.get_all_objects("house")
for house in all_houses:
    print(f"House {house['__name__']}: floor_area={house['floor_area']}")

# Get entire model as nested dictionary
model = gld.get_model()
for class_name, objects in model.items():
    print(f"{class_name}: {len(objects)} objects")
```

### Setting Properties

```python
import gridlabd

gld = gridlabd.GridLabD()
gld.load("model.glm")
gld.setup_after_load()

# Get objects
houses = gld.get_objects_by_class("house")

# Set a property value
if houses:
    result, value = gld.set_property(houses[0], "air_temperature", "72 degF")
    print(f"Set temperature result: {result}")
    
    # Verify the change
    result, new_value = gld.get_property(houses[0], "air_temperature")
    print(f"New temperature: {new_value}")
```

### Stepping Through Simulation

```python
import gridlabd

gld = gridlabd.GridLabD()
gld.load("model.glm")
gld.setup_after_load()

# Step through simulation timestep by timestep
for i in range(10):
    status, timestamp = gld.step()
    if status < 0:
        print("Simulation complete")
        break
    
    # Query state at each timestep
    houses = gld.get_all_objects("house")
    if houses:
        temp = houses[0].get('air_temperature')
        print(f"Timestep {i}, Time {timestamp}: Temperature = {temp}")
```

### Message Capture

```python
import gridlabd

gld = gridlabd.GridLabD()

# Enable message capture
gld.enable_message_capture(True)
gld.clear_messages()

# Load and run model
gld.load("model.glm")
gld.setup_after_load()
gld.run()

# Get captured messages
messages = gld.get_messages()
for msg in messages:
    print(f"[{msg['type']}] {msg['timestamp']}: {msg['message']}")

# Filter for errors only
errors = [m for m in messages if m['type'] == 'ERROR']
print(f"Found {len(errors)} errors")
```
   