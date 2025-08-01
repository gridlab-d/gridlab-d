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
    - A schema file containing the structure and metadata (TODO: needs work)
    
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
        # Directives filtering
        directives = raw.get('_directives', {})
        filtered_directives = {
            field_key: field_value
            for field_key, field_value in directives.items()
            if not ((isinstance(field_value, list) and not field_value) or
                    (isinstance(field_value, dict) and not field_value) or
                    (isinstance(field_value, str) and not field_value))
               and field_key not in ['item_cnt', 'entity', 'instances']
        }
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
        # Build modules and objects using parsed data
        # Modules: from module_entities.instances (single-instance)
        modules = {}
        for mtype, entity in model_file.module_entities.items():
            # Skip 'clock' and class definitions
            if mtype == 'clock' or mtype in classes:
                continue
            inst_dict = getattr(entity, 'instances', {})
            if inst_dict:
                # extract the single instance
                single = next(iter(inst_dict.values()), {})
                modules[mtype] = single
            # propagate any conditional-directive lists if present
            for attr in ('ifdef', 'ifndef', 'ifexist', 'if', 'ifnot'):
                vals = getattr(entity, attr, None)
                if vals:
                    modules[mtype][attr] = vals
        # Separate 'clock' as its own top-level entity
        clock_entity = model_file.module_entities.get('clock')
        clock_data = {}
        if clock_entity:
            insts = getattr(clock_entity, 'instances', {})
            if insts:
                clock_data = next(iter(insts.values()), {})
        # Objects: from model_file.model (type -> name -> params)
        objects = {}
        for otype, insts in model_file.model.items():
            obj_list = []
            for name, params in insts.items():
                entry = {'name': name}
                entry.update(params)
                obj_list.append(entry)
            if obj_list:
                objects[otype] = {'instances': obj_list}
        # Compose final JSON structure
        jsonEntity = {
            '_directives': filtered_directives,
            '__preamble': filtered_preamble,
            'classes': classes,
            'clock': clock_data,
            'modules': modules,
            'objects': objects,
            'schedules': raw.get('schedules', [])
        }

        # Dump filtered values JSON
        with open(output_file_path, 'w', encoding='utf-8') as op:
            json.dump(jsonEntity, op, ensure_ascii=False, indent=2)

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