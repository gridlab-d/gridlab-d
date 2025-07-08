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
    model_file = GLMModel()
    filePath = os.path.join(os.getcwd(), 'glmFiles', glmName + ".glm")
    if model_file.read_model(filePath):
        # Define the output directory and file path
        output_dir = os.path.join(os.getcwd(), 'output')
        output_file_path = os.path.join(output_dir, glmName + '_values.json')

        # Create the output directory if it doesn't exist
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        op = open(output_file_path, 'w', encoding='utf-8')
        jsonEntity = model_file.entities_to_json()

        # Filter: exclude unwanted fields and empty containers
        # For "directives": exclude item_cnt, entity, and instances
        # For other entities: exclude everything except instances and ifDef
        def should_keep_field(entity_key, field_key, field_value):
            # Check for empty containers first
            if (isinstance(field_value, list) and len(field_value) == 0) or \
               (isinstance(field_value, dict) and len(field_value) == 0) or \
               (isinstance(field_value, str) and len(field_value) == 0):
                return False
            
            if entity_key == "_directives":
                # For directives, exclude specific unwanted fields
                return field_key not in ["item_cnt", "entity", "instances"]
            else:
                # For other entities, only keep instances and ifDef
                return field_key in ["instances", "ifDef"]
        
        jsonEntity = {
            k: {
                field_key: field_value
                for field_key, field_value in v.items()
                if should_keep_field(k, field_key, field_value)
            }
            for k, v in jsonEntity.items()
            if k == "_directives" or (isinstance(v, dict) and "instances" in v)
        }

        # Filter: remove empty instances at the top level
        jsonEntity = {
            k: v
            for k, v in jsonEntity.items()
            if not (isinstance(v, dict) and len(v) == 0)
        }

        json.dump(jsonEntity, op, ensure_ascii=False, indent=2)
        op.close()

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