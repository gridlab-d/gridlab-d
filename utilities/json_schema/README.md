# GLM to JSON Converter

A Python utility for converting GridLAB-D model files (`.glm`) to JSON format, providing both structured data and schema outputs.

## Overview

This tool parses GridLAB-D model files and converts them into JSON format, making it easier to programmatically analyze and manipulate GridLAB-D models. It generates two types of output:
- **Values JSON**: Contains the actual model data with instances and configurations
- **Schema JSON**: Contains the structure and metadata of the model entities *(TODO: Schema generation needs further development)*

## Features

- Parses GridLAB-D `.glm` files including:
  - Objects (nodes, lines, transformers, etc.)
  - Modules and classes
  - Clock/date configurations
  - Directives (#include, #define, #set)
  - Schedules
  - Comments (inline, inside, and outside)
  - Conditional compilation (#ifdef/#endif)
- Generates clean JSON output with intelligent filtering
- Supports both standalone and package usage
- Command-line interface with customizable input files

## Directory Structure

```
glm_to_json/
├── README.md              # This file
├── glm_to_json.py         # Main converter script
├── glm_parser.py          # Core parsing logic
├── glm_entities.py        # Entity class definitions
├── glm_utils.py           # Utility functions
├── glmFiles/              # Input GLM files directory
├── output/                # Generated JSON files directory
├── references/            # Reference data (glm_classes.json)
└── __pycache__/           # Python cache files
```

## Requirements

- Python 3.6+
- Required packages:
  - `pyjson5`
  - `importlib_resources`

## Installation

1. Ensure you have Python 3.6 or later installed
2. Install required dependencies:
   ```bash
   pip install pyjson5 importlib_resources
   ```

## Usage

### Command Line

#### Basic Usage
```bash
python glm_to_json.py
```
This will process the default file `TE_CHALLENGE.glm` from the `glmFiles/` directory.

#### Custom GLM File
```bash
python glm_to_json.py your_model_name
```
This will process `your_model_name.glm` from the `glmFiles/` directory.

#### Examples
```bash
# Process the default TE_CHALLENGE.glm file
python glm_to_json.py

# Process a custom model file named "residential_feeder.glm"
python glm_to_json.py residential_feeder

# Process a model with specific name
python glm_to_json.py IEEE_13_Node_Test_Feeder
```

### Python Package Usage

```python
from glm_to_json import glm_to_json

# Convert default model
glm_to_json()

# Convert specific model
glm_to_json("your_model_name")
```

## Input Requirements

1. **GLM File Location**: Place your `.glm` files in the `glmFiles/` directory
2. **File Naming**: The script expects files with `.glm` extension
3. **File Format**: Standard GridLAB-D model file format

## Output

The converter generates two JSON files in the `output/` directory:

### 1. Values File (`{model_name}_values.json`)
Contains the actual model data including:
- Object instances with their properties
- Module configurations
- Directives (includes, defines, sets)
- Conditional compilation blocks (ifdef)

Example structure:
```json
{
  "_directives": {
    "includes": ["path/to/included/file.glm"],
    "defines": ["VARIABLE_NAME value"],
    "sets": ["property=value"]
  },
  "powerflow": {
    "instances": {
      "module_instance": {
        "solver_method": "NR"
      }
    }
  },
  "node": {
    "instances": {
      "node_1": {
        "phases": "ABCN",
        "voltage_A": "7200+0j V"
      }
    }
  }
}
```

### 2. Schema File (`{model_name}_schema.json`) - **TODO**
Contains the structure and metadata for all entity types in the model.

> **Note**: Schema generation is currently under development and may not produce complete or accurate results. This feature requires additional work to properly extract and format entity schemas from the GLM class definitions.

## Filtering Logic

The converter applies intelligent filtering to keep the output clean:

- **Empty containers**: Removes empty lists, dictionaries, and strings
- **Directives**: Keeps only meaningful directive content
- **Entities**: Focuses on instances and conditional compilation blocks
- **Top-level filtering**: Removes completely empty entity groups

## Error Handling

- **File Not Found**: The script will raise a `FileNotFoundError` if the specified GLM file doesn't exist
- **Parse Errors**: Unrecognized lines are printed to console but don't stop processing
- **Import Errors**: Graceful fallback from relative to absolute imports

## Troubleshooting

### Common Issues

1. **ModuleNotFoundError**: Ensure all required packages are installed
2. **FileNotFoundError**: Check that your GLM file exists in the `glmFiles/` directory
3. **Empty Output**: Verify your GLM file has valid GridLAB-D syntax

### Debug Tips

- Check the console output for "Un-parsed line" messages
- Verify your GLM file syntax using GridLAB-D directly
- Ensure proper file encoding (UTF-8 recommended)

## Supported GridLAB-D Elements

- **Objects**: All standard GridLAB-D objects (nodes, lines, transformers, etc.)
- **Modules**: powerflow, residential, commercial, etc.
- **Directives**: #include, #define, #set
- **Conditional Compilation**: #ifdef, #endif
- **Schedules**: Named schedule definitions
- **Classes**: User-defined classes
- **Comments**: Inline (//) and block comments

## Examples

### Example 1: Processing a Simple Feeder
```bash
# Assuming you have "simple_feeder.glm" in glmFiles/
python glm_to_json.py simple_feeder
```

### Example 2: Batch Processing
```python
models = ["feeder_1", "feeder_2", "feeder_3"]
for model in models:
    glm_to_json(model)
```

## Contributing

When modifying this tool:
1. Follow the existing code structure
2. Add appropriate error handling
3. Update this README for any new features
4. Test with various GLM file formats

## License

This tool is part of the GridLAB-D project. See the main project LICENSE file for details.
