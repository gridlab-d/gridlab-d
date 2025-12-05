"""
GLM to JSON conversion utility.

Converts GridLAB-D model (.glm) files to JSON. Supports single file and batch
processing with in-place progress updates. Conditional directives (#ifdef, etc.)
are not supported and will raise GLMConditionalError.
"""

import os
import sys
import json
import argparse
from pathlib import Path
from typing import List, Tuple

# Add the current directory to the Python path to ensure local imports work
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

# Import the modular components
try:
    # Try relative imports first (for package usage)
    from .glm_parser import GLMModel, GLMConditionalError
except (ImportError, ValueError):
    # Fall back to direct imports (for standalone usage)
    from glm_parser import GLMModel, GLMConditionalError

def glm_to_json(glm_name, input_dir=None, output_dir=None, output_name=None):
    """Convert a GLM file to JSON format.
    
    Args:
        glm_name (str): Name of the GLM file (without .glm extension)
        input_dir (str, optional): Input directory (default: 'glmFiles/')
        output_dir (str, optional): Output directory (default: 'output/' or same as input_dir)
        output_name (str, optional): Custom output filename (without .json extension, defaults to glm_name)
    
    Returns:
        bool: True if successful, False otherwise.
    """
    model_file = GLMModel()
    if not glm_name:
        print("\n❌ Error: No GLM file name provided")
        print("\n💡 Run with --help for usage information")
        return False
    
    # Default output name to input name if not specified
    if output_name is None:
        output_name = glm_name
    
    # Determine input directory
    if input_dir:
        input_path = Path(input_dir)
    else:
        input_path = Path.cwd() / 'glmFiles'
    
    filePath = input_path / f"{glm_name}.glm"
    
    try:
        success = model_file.read_model(str(filePath))
    except GLMConditionalError as e:
        print(f"\n❌ Error: {str(e)}")
        print("\n💡 Run with --help for usage information")
        return False
    except FileNotFoundError:
        print("\n❌ Error: GLM file not found")
        print(f"📄 Expected location: {filePath}")
        if not input_dir:
            print("💡 Please ensure the GLM file exists in the 'glmFiles/' directory")
            print("   or use --dir to specify a custom directory")
        else:
            print(f"💡 Please ensure the GLM file exists in: {input_path}")
        print("\n💡 Run with --help for usage information")
        return False
    except Exception as e:
        print(f"\n❌ Error reading GLM file: {str(e)}")
        print(f"📄 File: {filePath}")
        print("\n💡 Run with --help for usage information")
        return False
    
    if success:
        # Determine output directory
        # If input_dir was specified, output to same directory by default
        # Otherwise use 'output/' directory
        if output_dir:
            out_path = Path(output_dir)
        elif input_dir:
            out_path = input_path
        else:
            out_path = Path.cwd() / 'output'
        
        # Create the output directory if it doesn't exist
        out_path.mkdir(parents=True, exist_ok=True)
        
        output_file_path = out_path / f"{output_name}.json"

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
            # Skip 'clock' only (classes are now in class_entities, not module_entities)
            if mtype == 'clock':
                continue
            inst_dict = getattr(entity, 'instances', {})
            if inst_dict:
                # Extract the single instance and ensure numeric conversion in its values
                single = next(iter(inst_dict.values()), {})
                modules[mtype] = try_conversion(single)
                # Add entity-level conditionals only if instance doesn't already have them
                # (instance-level conditionals are more accurate after merging)
                if entity._conditionals and '_conditionals' not in modules[mtype]:
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
            'schedules': raw.get('schedules', {})
        }

        # Remove empty blocks
        jsonEntity = {k: v for k, v in jsonEntity.items() if v}

        # Dump filtered values JSON
        with open(output_file_path, 'w', encoding='utf-8') as op:
            json.dump(jsonEntity, op, ensure_ascii=False, indent=2)
        
        print("✅ Successfully converted GLM to JSON")
        print(f"📄 Input:  {filePath}")
        print(f"📄 Output: {output_file_path}")
        return True
    else:
        print("\n❌ Error: Failed to read GLM file")
        print(f"📄 File: {filePath}")
        print("💡 Please check the file format and try again")
        print("\n💡 Run with --help for usage information")
        return False

def find_glm_files(base_dir, autotest_only=True):
    """Find .glm files in autotest directories or recursively.
    
    Args:
        base_dir (str or Path): Base directory to search
        autotest_only (bool): Search only autotest/ directories (True) or all directories (False)
        
    Returns:
        List[Path]: Sorted absolute paths to .glm files.
    """
    base_path = Path(base_dir)
    glm_files = []
    
    if autotest_only:
        # Find all directories named 'autotest'
        for autotest_dir in base_path.rglob('autotest'):
            if autotest_dir.is_dir():
                # Find all .glm files directly in the autotest directory (not in subdirectories)
                for glm_file in autotest_dir.glob('*.glm'):
                    glm_files.append(glm_file)
    else:
        # Find all .glm files recursively
        glm_files = list(base_path.rglob('*.glm'))
    
    return sorted(glm_files)

def convert_batch_files(search_dir=None, output_dir=None):
    """Convert multiple GLM files with in-place progress updates.
    
    Args:
        search_dir (str, optional): Custom directory for recursive search (default: autotest/*/)
        output_dir (str, optional): Output directory for all JSON (default: alongside GLM files)
        
    Returns:
        Tuple[int, int, List[str]]: (total_count, success_count, error_files)
    """
    # Determine search behavior
    if search_dir:
        # Custom directory: search all .glm files recursively
        search_path = Path(search_dir)
        autotest_only = False
        print(f"🔍 Recursively searching for all .glm files under: {search_path}")
    else:
        # Default: search autotest directories from repository root
        search_path = Path(__file__).parent.parent.parent
        autotest_only = True
        print(f"🔍 Searching for .glm files in autotest directories under: {search_path}")
    
    # Find .glm files
    glm_files = find_glm_files(search_path, autotest_only=autotest_only)
    
    if not glm_files:
        if autotest_only:
            print("❌ No .glm files found in autotest directories")
        else:
            print("❌ No .glm files found")
        return 0, 0, []
    
    if autotest_only:
        print(f"📁 Found {len(glm_files)} .glm files in autotest directories\n")
    else:
        print(f"📁 Found {len(glm_files)} .glm files\n")
    
    success_count = 0
    error_files = []
    
    for idx, glm_file in enumerate(glm_files, 1):
        glm_name = glm_file.stem  # filename without extension
        glm_dir = glm_file.parent
        
        # Determine output location
        if output_dir:
            out_dir = output_dir
        else:
            # Output in the same directory as the .glm file
            out_dir = str(glm_dir)
        
        # Determine output filename
        # Files starting with "test_" get "_converted" added before the extension
        if glm_name.startswith('test_'):
            output_name = f"{glm_name}_converted"
        else:
            output_name = glm_name
        
        # Display progress on a single line that updates
        rel_path = glm_file.relative_to(search_path)
        # Clear line and show progress
        print(f"\r\033[K🔄 [{idx}/{len(glm_files)}] Converting: {rel_path}", end='', flush=True)
        
        # Convert using existing function, passing full path as input_dir
        # Temporarily suppress output from glm_to_json during batch mode
        import io
        from contextlib import redirect_stdout, redirect_stderr
        
        f = io.StringIO()
        with redirect_stdout(f), redirect_stderr(f):
            result = glm_to_json(glm_name, str(glm_dir), out_dir, output_name)
        
        if result:
            success_count += 1
            # Show success on the same line
            print(f"\r\033[K✅ [{idx}/{len(glm_files)}] Converted: {rel_path}", end='', flush=True)
        else:
            error_files.append(str(rel_path))
            # Show error and move to next line since errors need to be visible
            print(f"\r\033[K❌ [{idx}/{len(glm_files)}] Failed: {rel_path}")
    
    # Move to next line after all conversions complete
    print()
    
    return len(glm_files), success_count, error_files

        # Press the green button in the gutter to run the script.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Convert GridLAB-D model files (.glm) to JSON format',
        epilog='''
Examples:
  Single file conversion:
    %(prog)s mymodel
        Convert glmFiles/mymodel.glm to output/mymodel.json
    
    %(prog)s mymodel --dir /path/to/models
        Convert /path/to/models/mymodel.glm to /path/to/models/mymodel.json
    
    %(prog)s mymodel --dir /path/to/models --output /path/to/output
        Convert /path/to/models/mymodel.glm to /path/to/output/mymodel.json
  
  Batch conversion:
    %(prog)s --batch
        Find and convert all .glm files in autotest directories from project root (in place)
    
    %(prog)s --batch --search /custom/path
        Recursively find and convert ALL .glm files under /custom/path (in place)
    
    %(prog)s --batch --output /path/to/output
        Convert autotest .glm files and save all to specified output directory
    
    %(prog)s --batch --search /custom/path --output /path/to/output
        Recursively convert all .glm files and save to output directory
        ''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('glm_name',
                        nargs='?',
                        metavar='GLM_NAME',
                        help='Name of the GLM file (without .glm extension)')
    
    parser.add_argument('-d', '--dir',
                        dest='input_dir',
                        metavar='DIR',
                        help='Directory containing the GLM file (default: glmFiles/)')
    
    parser.add_argument('-o', '--output',
                        dest='output_dir',
                        metavar='DIR',
                        help='Directory for output JSON file (default: output/, or same as --dir if specified)')
    
    parser.add_argument('--batch',
                        action='store_true',
                        help='Batch mode: convert all .glm files in autotest directories (default), or all .glm files recursively if --search is specified')
    
    parser.add_argument('--search',
                        dest='search_dir',
                        metavar='DIR',
                        help='Directory to recursively search for .glm files (used with --batch). When specified, searches ALL .glm files, not just autotest directories')
    
    args = parser.parse_args()
    
    # Handle batch conversion mode
    if args.batch:
        total, success, errors = convert_batch_files(args.search_dir, args.output_dir)
        
        print("=" * 60)
        print("📊 CONVERSION SUMMARY")
        print("=" * 60)
        print(f"Total files found:     {total}")
        print(f"Successfully converted: {success}")
        print(f"Failed:                {len(errors)}")
        
        if errors:
            sys.exit(1)
        else:
            print("\n✅ All files converted successfully!")
            sys.exit(0)
    
    # Handle single file conversion mode
    if not args.glm_name:
        parser.error("GLM_NAME is required when not using --batch")
    
    conversion_result = glm_to_json(args.glm_name, args.input_dir, args.output_dir)
    if conversion_result is False:
        sys.exit(1)