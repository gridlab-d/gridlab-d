# GLM to JSON Converter

Converts GridLAB-D model files (`.glm`) to JSON format for easier programmatic analysis and manipulation.

## Features

- Parses objects, modules, classes, clock, directives, schedules, and comments
- Single file and batch conversion modes
- Batch mode: autotest directory scanning or recursive search
- Proper array handling and automatic type conversion
- Clean JSON output with intelligent filtering
- JSON Schema generation for validation and IDE support

## Directory Structure

```
json_schema/
├── README.md              # This file
├── glm_to_json.py         # Main converter script with CLI
├── generate_schema.py     # JSON Schema generator
├── glm_parser.py          # Core parsing logic and model management
├── glm_entities.py        # Entity and Item class definitions
├── glm_utils.py           # Utility functions
├── references/            # Reference data (glm_classes.json)
│   └── glm_classes.json   # Class definitions reference
├── glm_schema.json        # Generated JSON Schema (created by generate_schema.py)
└── __pycache__/           # Python cache files
```

## Requirements

- Python 3.6+
- Required packages:
  - `pyjson5`
  - `importlib_resources`
- Optional (for schema validation):
  - `jsonschema`

## Installation

```bash
pip install pyjson5 importlib_resources jsonschema
```

Requires Python 3.6+.

## Usage

### Converting GLM Files to JSON

#### Command Line

```bash
# Single file (output to same directory)
python glm_to_json.py mymodel --dir /path/to/models

# Single file (custom output)
python glm_to_json.py mymodel --dir /path/to/models --output /path/to/output

# Batch: all autotest files
python glm_to_json.py --batch

# Batch: custom directory (recursive)
python glm_to_json.py --batch --search /custom/path

# Batch: custom output directory
python glm_to_json.py --batch --output /path/to/output
```

### Batch Mode

- Real-time progress with in-place console updates
- Error tracking and summary report
- Default: searches `autotest/*/*.glm` from repository root  
- With `--search`: recursively searches all `.glm` files in directory

### Python Package Usage

```python
from glm_to_json import glm_to_json

# Convert with default directories (glmFiles/ -> output/)
glm_to_json("your_model_name")

# Convert with custom directories
glm_to_json("your_model_name", input_dir="/path/to/models", output_dir="/path/to/output")

# Batch conversion
from glm_to_json import convert_batch_files
total, success, errors = convert_batch_files()
print(f"Converted {success}/{total} files")
```

## Output Structure

Generates a single JSON file with:
- `__preamble`: Comments before first module/object
- `_directives`: `#include`, `#define`, `#set`, `#undef`  
- `_legacy`: `#setenv`, `#binpath`, `#libpath`, `#incpath`, `#option`, `#system`, `#start`
- `clock`: Timezone and timestamps
- `modules`: Module configurations
- `classes`: User-defined classes
- `objects`: Object instances grouped by type
- `schedules`: Schedule definitions

See full JSON example in previous version of this README if needed.

## Processing

- Removes empty containers and metadata fields
- Converts numeric strings to int/float
- Properly formats comment arrays (not string representations)
- By default, creates JSON alongside source GLM files

## Error Handling

**Unsupported**: Conditional directives (`#ifdef`, `#ifndef`, `#ifexist`, `#if`, `#else`, `#endif`) will raise `GLMConditionalError`. These must be manually resolved before conversion.

**Common issues**:
- FileNotFoundError: Check file path and directory
- Parse errors: Check console for "Unrecognized parameter" messages  
- Empty output: Verify valid GLM syntax
- Permission errors: Ensure write permissions for output directory

## Limitations

**Parsed but not evaluated**:
- `#include`: Recorded but files not merged
- `#define`: Recorded but macros not expanded

**Not supported**:
- Conditional directives: `#ifdef`, `#ifndef`, `#ifexist`, `#if`, `#else`, `#endif`
- Complex class hierarchies

## Examples

```bash
# Single file
python glm_to_json.py IEEE-123 --dir ./tests

# Batch: all autotest files
python glm_to_json.py --batch

# Batch: custom directory
python glm_to_json.py --batch --search /path/to/models
```

```python
# Python API
from glm_to_json import glm_to_json, convert_batch_files

glm_to_json("mymodel", input_dir="./models", output_dir="./json")
total, success, errors = convert_batch_files(search_dir="./models")
```

### Generating JSON Schema

The `generate_schema.py` tool creates a JSON Schema that describes the structure of JSON files produced by `glm_to_json.py`.

#### Command Line

```bash
# Generate schema with default name (glm_schema.json)
python generate_schema.py

# Generate schema with custom name
python generate_schema.py --output custom_schema.json

# Generate schema in specific directory
python generate_schema.py --output /path/to/output/schema.json
```

#### Python API

```python
from generate_schema import generate_schema

# Generate with default name
generate_schema()

# Generate with custom path
generate_schema("path/to/schema.json")
```

#### Using the Generated Schema

The generated JSON Schema can be used to:
- **Validate JSON files** with validation tools
- **Enable IDE autocomplete** in VS Code, IntelliJ, etc.
- **Document the JSON format** for API consumers

**Validation with Python:**
```python
import json
from jsonschema import validate

# Load schema and data
with open('glm_schema.json') as f:
    schema = json.load(f)
with open('mymodel.json') as f:
    data = json.load(f)

# Validate
validate(instance=data, schema=schema)
print("✅ Valid!")
```

**VS Code Integration:**
Add to your workspace `.vscode/settings.json`:
```json
{
  "json.schemas": [
    {
      "fileMatch": ["*.json"],
      "url": "./glm_schema.json"
    }
  ]
}
```

## License

Part of the GridLAB-D project. See main project LICENSE.
