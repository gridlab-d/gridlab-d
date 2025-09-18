"""
GLM to JSON Converter.

This module provides functionality to convert GridLAB-D model files (.glm)
to JSON format, generating both values and schema files for easier 
programmatic analysis and manipulation of GridLAB-D models.
"""

import os
import sys
import json
import argparse

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

def glm_to_json(glmName="TE_CHALLENGE"):
    """Convert a GLM file to JSON format.
    
    Reads a GridLAB-D model file and converts it to two JSON files:
    - A values file containing the actual model data and instances
    - A schema file containing the structure and metadata
    
    Args:
        glmName (str): Name of the GLM file (without .glm extension).
                      Defaults to "TE_CHALLENGE".
                      
    Note:
        The GLM file should be located in the 'glmFiles/' directory relative
        to the current working directory. Output files are saved to the 
        'output/' directory.
    """
    model_file = GLMModel()
    filePath = os.path.join(os.getcwd(), 'glmFiles', glmName + ".glm")
    if model_file.read_model(filePath):
        # Define the output directory and file path
        output_dir = os.path.join(os.getcwd(), 'output')
        output_file_path = os.path.join(output_dir, glmName + '_values.json')

        # Create the output directory if it doesn't exist
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        # Extract class blueprints from parsed GLM definitions
        classes = getattr(model_file, 'class_definitions', {})
        # Generate full entities JSON and remove class entries to avoid object overrides
        raw = model_file.entities_to_json()
        for ctype in classes:
            raw.pop(ctype, None)

        def filter_and_transform(directives, excluded_keys):
            # Filter and transform the directives dictionary
            filtered = {
                field_key: field_value
                for field_key, field_value in directives.items()
                if field_key not in excluded_keys and field_value and (not isinstance(field_value, dict) or field_value)
            }

            return filtered
        # Keys to exclude during filtering
        excluded_keys = ['item_cnt', 'entity', 'instances']

        # Process '_directives'
        directives = raw.get('_directives', {})
        filtered_directives = filter_and_transform(directives, excluded_keys)

        # Process '_legacy'
        legacy_directives = raw.get('_legacy', {})
        filtered_legacy = filter_and_transform(legacy_directives, excluded_keys)

        # Add preamble comments if available
        preamble = raw.get('__preamble', {})
        filtered_preamble = {
            field_key: field_value
            for field_key, field_value in preamble.items()
            if not ((isinstance(field_value, list) and not field_value) or
                    (isinstance(field_value, dict) and not field_value) or
                    (isinstance(field_value, str) and not field_value))
               and field_key not in ['item_cnt', 'entity', 'instances']
        }

        # TODO: Come back to this when schema v1 is ready and check against it for which fields to do number conversion
        def try_conversion(value):
            """
            Try to convert a string to a number type (int or float) or boolean.
            """
            if isinstance(value, str):
                if value.lower() == "true":
                    return True
                elif value.lower() == "false":
                    return False
                try:
                    # Try converting to an integer first
                    return int(value)
                except ValueError:
                    try:
                        # Try converting to a float if integer conversion fails
                        return float(value)
                    except ValueError:
                        # Return the original string if neither conversion is possible
                        return value
            elif isinstance(value, dict):
                # Recursively convert values in a dictionary
                return {k: try_conversion(v) for k, v in value.items()}
            elif isinstance(value, list):
                # Recursively convert values in a list
                return [try_conversion(v) for v in value]
            return value  # Return the original value if not a string


        # Build modules and objects using parsed data
        modules = {}
        for mtype, entity in model_file.module_entities.items():
            # Skip 'clock' and class definitions
            if mtype == 'clock' or mtype in classes:
                continue
            inst_dict = getattr(entity, 'instances', {})
            if inst_dict:
                # Extract the single instance and ensure numeric conversion in its values
                single = next(iter(inst_dict.values()), {})
                modules[mtype] = try_conversion(single)
                #check if there are conditionals and add them
                if entity._conditionals:
                    modules[mtype]["_conditionals"] = entity._conditionals

        # Separate 'clock' as its own top-level entity
        clock_entity = model_file.module_entities.get('clock')
        clock_data = {}
        if clock_entity:
            insts = getattr(clock_entity, 'instances', {})
            if insts:
                clock_data = try_conversion(next(iter(insts.values()), {}))

        # Build objects using parsed data
        objects = {}
        for otype, insts in model_file.model.items():
            obj_list = []
            for name, params in insts.items():
                entry = {'name': name}
                try:
                    entry.update(try_conversion(params))
                except Exception as e:
                    print(f"Error processing {name}: {e}")  
                obj_list.append(entry)
            if obj_list:
                objects[otype] = {'instances': obj_list}

        # Compose final JSON structure
        jsonEntity = {
            '__preamble': filtered_preamble,
            '_directives': filtered_directives,
            '_legacy': filtered_legacy,
            'classes': classes,
            'clock': clock_data,
            'modules': modules,
            'objects': objects,
            'schedules': raw.get('schedules', [])
        }

        # Remove empty blocks
        jsonEntity = {k: v for k, v in jsonEntity.items() if v}

        # Dump filtered values JSON
        with open(output_file_path, 'w', encoding='utf-8') as op:
            json.dump(jsonEntity, op, ensure_ascii=False, indent=2)
        return # Disable schema creation for now
        # Define the output directory and file path
        output_dir = os.path.join(os.getcwd(), 'output')
        output_file_path = os.path.join(output_dir, glmName + '_schema.json')

        # Create the output directory if it doesn't exist
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        op = open(output_file_path, 'w', encoding='utf-8')
        json.dump(model_file.entities_to_schema(), op, ensure_ascii=False, indent=2)
        op.close()

        # Press the green button in the gutter to run the script.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert GLM file to JSON format')
    parser.add_argument('glmName', nargs='?', default='TE_CHALLENGE', 
                       help='Name of the GLM file (without .glm extension). Default: TE_CHALLENGE')
    
    args = parser.parse_args()
    glm_to_json(args.glmName)