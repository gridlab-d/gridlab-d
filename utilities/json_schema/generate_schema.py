"""
JSON Schema Generator for GridLAB-D GLM to JSON Conversion.

This tool generates a JSON Schema file that describes the structure of JSON files
created by the glm_to_json.py converter. The schema is based on GridLAB-D class
definitions from glm_classes.json and the GLMModel entity structure.

The generated schema can be used for:
- Validating converted GLM JSON files
- IDE autocomplete and validation
- Documentation of the JSON format
"""

import os
import sys
import json
import argparse
from pathlib import Path

# Add the current directory to the Python path to ensure local imports work
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

# Import the modular components
try:
    # Try relative imports first (for package usage)
    from .glm_parser import GLMModel
except (ImportError, ValueError):
    # Fall back to direct imports (for standalone usage)
    from glm_parser import GLMModel


def generate_schema(output_file=None):
    """Generate JSON Schema from GLMModel entity definitions.
    
    Creates a JSON Schema that describes the structure of JSON files produced
    by glm_to_json.py. The schema includes:
    - Top-level structure (__preamble, _directives, _legacy, classes, clock, modules, objects, schedules)
    - Module definitions
    - Object type definitions with instances
    - Class definitions
    
    Args:
        output_file (str, optional): Path to output schema file (default: glm_schema.json)
        
    Returns:
        bool: True if successful, False otherwise.
    """
    # Default output file
    if output_file is None:
        output_file = "glm_schema.json"
    
    output_path = Path(output_file)
    
    print(f"🔧 Generating JSON Schema from GridLAB-D class definitions...")
    
    try:
        # Create a GLMModel instance to access entity definitions
        model = GLMModel()
        
        # Generate the schema using the GLMModel's entities_to_schema method
        schema = model.entities_to_schema()
        
        # Write the schema to file
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(schema, f, ensure_ascii=False, indent=2)
        
        print(f"✅ Successfully generated JSON Schema")
        print(f"📄 Output: {output_path.absolute()}")
        print(f"📊 Schema includes:")
        print(f"   - Top-level structure definitions")
        print(f"   - Module entity schemas")
        print(f"   - Object entity schemas")
        print(f"   - Conditional directive definitions")
        return True
        
    except Exception as e:
        print(f"\n❌ Error generating schema: {str(e)}")
        print(f"💡 Please ensure glm_classes.json is available in the references/ directory")
        import traceback
        traceback.print_exc()
        return False


def main():
    """Main entry point for the schema generator."""
    parser = argparse.ArgumentParser(
        description='Generate JSON Schema for GridLAB-D GLM to JSON conversion format',
        epilog='''
Examples:
  Generate schema with default name (glm_schema.json):
    %(prog)s
  
  Generate schema with custom name:
    %(prog)s --output custom_schema.json
  
  Generate schema in specific directory:
    %(prog)s --output /path/to/output/schema.json
        ''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('-o', '--output',
                        dest='output_file',
                        metavar='FILE',
                        help='Output schema file path (default: glm_schema.json)')
    
    args = parser.parse_args()
    
    result = generate_schema(args.output_file)
    
    if not result:
        sys.exit(1)
    
    print("\n💡 You can use this schema to validate JSON files with tools like:")
    print("   - jsonschema (Python): pip install jsonschema")
    print("   - ajv (Node.js): npm install ajv")
    print("   - VS Code JSON validation (add to settings.json)")


if __name__ == '__main__':
    main()
