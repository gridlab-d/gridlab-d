"""
GLM parsing logic and model management.

This module provides the core functionality for parsing GridLAB-D model files
(.glm) and converting them to structured data representations. It handles
the parsing of various GLM constructs including objects, modules, schedules,
directives, and comments.

The main class GLMModel manages the entire parsing process and maintains
the parsed model structure with support for JSON serialization and schema
generation.
"""

import os
import re
import pyjson5
from importlib_resources import files

# Import the modular components with fallback
try:
    # Try relative imports first (for package usage)
    from .glm_entities import Entity, O_Entity, GLM
    from .glm_utils import gld_strict_name, add_attr_to_entity
except (ImportError, ValueError):
    # Fall back to direct imports (for standalone usage)
    from glm_entities import Entity, O_Entity, GLM
    from glm_utils import gld_strict_name, add_attr_to_entity

glm_entities_path = files('references').joinpath('glm_classes.json')

class GLMModel:
    """Main class for parsing and managing GridLAB-D model files.
    
    This class handles the parsing of GLM files, manages entities and objects,
    and provides methods for converting the parsed model to JSON format.
    It maintains collections of modules, objects, schedules, and various
    directives found in GLM files.
    
    """
    
    def __init__(self):
        """Initialize a new GLMModel instance.
        
        Loads class definitions from the reference JSON file and sets up
        the basic entity structure including clock and directives entities.
        """
        self.hash = None
        self.root = None
        self.in_file = ""
        self.out_file = ""
        self.model = {}
        self.glm = GLM()
        self.conn = None
        self.modules = None
        self.objects = None
        self.schedule_types = {}
        self.module_types = []
        self.class_types = []
        self.module_entities = {}
        self.object_entities = {}
        self.set_lines = []
        self.define_lines = []
        self.undef_lines = []
        self.error_lines = []
        self.outside_errors = []
        self.include_lines = []
        self.ifdef_lines = []
        self.class_definitions = {}
        self.configuration = {}
        self.inside_comments = dict()
        self.outside_comments = dict()
        self.inline_comments = dict()
        self.warning_lines = []
        self.outside_warnings = []
        self.print_lines = []
        self.outside_prints = []
        # top-level extended directives
        self.setenv_lines = []
        self.outside_setenvs = []
        self.binpath_lines = []
        self.outside_binpaths = []
        self.libpath_lines = []
        self.outside_libpaths = []
        self.incpath_lines = []
        self.outside_incpaths = []
        self.option_lines = []
        self.outside_options = []
        self.system_lines = []
        self.outside_systems = []
        self.start_lines = []
        self.outside_starts = []
        
        with open(glm_entities_path, 'r', encoding='utf-8') as json_file:
            self.classes = pyjson5.load(json_file)
            entity = Entity("clock", None)
            entity.add_attr("TEXT", "Time zone", "", "timezone", value=None)
            entity.add_attr("TEXT", "Start time", "", "timestamp", value=None)
            entity.add_attr("TEXT", "Start time", "", "starttime", value=None)
            entity.add_attr("TEXT", "Stop time", "", "stoptime", value=None)
            self.module_entities["clock"] = entity
            entity = Entity("_directives", None)
            entity.add_attr("TEXTARRAY", "#include", "", "includes", value=[])
            entity.add_attr("TEXTARRAY", "#define", "", "defines", value=[])
            entity.add_attr("TEXTARRAY", "#set", "", "sets", value=[])
            entity.add_attr("TEXTARRAY", "#undef", "", "undefs", value=[])
            self.module_entities["_directives"] = entity
            entity = Entity("_legacy", None)
            entity.add_attr("TEXTARRAY", "#setenv", "", "setenvs", value=[])
            entity.add_attr("TEXTARRAY", "#binpath", "", "binpaths", value=[])
            entity.add_attr("TEXTARRAY", "#libpath", "", "libpaths", value=[])
            entity.add_attr("TEXTARRAY", "#incpath", "", "incpaths", value=[])
            entity.add_attr("TEXTARRAY", "#option", "", "options", value=[])
            entity.add_attr("TEXTARRAY", "#system", "", "systems", value=[])
            entity.add_attr("TEXTARRAY", "#start", "", "starts", value=[])
            self.module_entities["_legacy"] = entity
            entity = Entity("__preamble", None)
            entity.add_attr("TEXTARRAY", "Preamble comments", "", "comments", value=[])
            self.module_entities["__preamble"] = entity
            for module_name in self.classes:
                self.module_types.append(module_name)
                for object_name in self.classes[module_name]:
                    obj = self.classes[module_name][object_name]
                    if obj is None:
                        self.classes[module_name][object_name] = {}
                        continue
                    if object_name == "global_attributes":
                        entity = Entity(module_name, None)
                        for attr in obj:
                            add_attr_to_entity(entity, attr, obj[attr])
                        self.module_entities[module_name] = entity
                    else:
                        obj = self.classes[module_name][object_name]
                        entity = O_Entity(self, object_name, None)
                        entity.add_attr("OBJECT", "Parent", "", "parent", value=None)
                        for attr in obj:
                            add_attr_to_entity(entity, attr, obj[attr])
                        self.object_entities[object_name] = entity
                        setattr(self.glm, object_name, entity)

    def set_module_instance(self, mod_type, params):
        """Create and configure a module instance.
        
        Args:
            mod_type (str): The module type name
            params (dict): Parameters for the module instance
            
        Returns:
            Entity instance or None: The created module instance
            
        Raises:
            TypeError: If mod_type is not a string
        """
        if isinstance(mod_type, str):
            try:
                entity = self.module_entities[mod_type]
                return entity.set_instance(mod_type, params)
            except KeyError:
                print(f"Unrecognized GRIDLABD module: {mod_type}, "
                      "must be a new class")
                self.class_types.append(mod_type)
                entity = Entity(mod_type, None)
                self.module_entities[mod_type] = entity
                for items in params:
                    if items in ["integer", "double", "string"]:
                        entity.add_attr('TEXT', items[0], "", items[0], "")
                return entity.set_instance(mod_type, params)
        else:
            raise TypeError(f"{mod_type} must be a string and is not.")
        return None

    def set_object_instance(self, obj_type, object_name, params):
        """Create and configure an object instance.
        
        Args:
            obj_type (str): The object type name
            object_name (str): The name for the object instance
            params (dict): Parameters for the object instance
            
        Returns:
            Entity instance or None: The created object instance
            
        Raises:
            TypeError: If obj_type or object_name is not a string
        """
        if isinstance(obj_type, str) and isinstance(object_name, str):
            try:
                entity = self.object_entities[obj_type]
            except KeyError:
                print(f"Unrecognized GRIDLABD object and id: {obj_type} "
                      f"{object_name}, must be a new object")
                if obj_type in self.class_types:
                    entity = O_Entity(obj_type, self.objects[obj_type])
                    self.object_entities[obj_type] = entity
                    for items in params:
                        entity.add_attr('TEXT', items[0], "", items[0], "")
                else:
                    print(f"Unrecognized user class/object and id: "
                          f"{obj_type} {object_name}")
                    return None
            return entity.set_instance(object_name, params)
        else:
            raise TypeError(f"GRIDLABD object type and/or object name "
                           f"{obj_type} must be a string and is not.")

    def entities_to_json(self):
        """Convert all entities to JSON format.
        
        Returns:
            dict: Dictionary containing JSON representations of all entities
        """
        diction = {}
        for name in self.module_entities:
            value = self.module_entities[name].to_json()
            if value is not None:
                diction[name] = self.module_entities[name].to_json()
        for name in self.object_entities:
            value = self.object_entities[name].to_json()
            if value is not None:
                diction[name] = self.object_entities[name].to_json()
        if hasattr(self, 'error_lines') and self.error_lines:
            # top-level error directives
            diction['trigger_error'] = list(self.error_lines)
        if hasattr(self, 'warning_lines') and self.warning_lines:
            # top-level warning directives
            diction['trigger_warning'] = list(self.warning_lines)
        if hasattr(self, 'print_lines') and self.print_lines:
            # top-level print directives
            diction['print'] = list(self.print_lines)
        # include schedule definitions
        if hasattr(self, 'schedule_types') and self.schedule_types:
            schedules = []
            for sched_name, lines in self.schedule_types.items():
                text = '\n'.join(lines)
                # extract content between braces
                blocks_raw = re.findall(r'\{([^}]*)\}', text, re.DOTALL)
                blocks = []
                for raw_block in blocks_raw:
                    lines_blk = [l.strip().rstrip(';') for l in raw_block.splitlines()]
                    # derive name from first comment, default empty
                    name = ""
                    items = []
                    for l in lines_blk:
                        if l.startswith('//'):
                            name = l[2:].strip()
                        elif l.startswith('*'):
                            items.append(l)
                    blocks.append({'name': name, 'items': items})
                schedules.append({sched_name: blocks})
            diction['schedules'] = schedules
        return diction

    def entities_to_schema(self):
        """
        Convert all entities to a JSON Schema format, properly nesting `modules` and `objects`.

        Creates top-level categories (like `modules` and `objects`) in the schema to group related data.

        Returns:
            dict: A single JSON Schema object describing all entities.
        """
        schema = {
            "$schema": "https://json-schema.org/draft/2020-12/schema",  # Declaration ONLY at the top-level
            "type": "object",
            "properties": {
                "modules": {  # Group all `modules` in a single object
                    "type": "object",
                    "properties": {},
                    "required": [],
                    "additionalProperties": True
                },
                "objects": {  # Group all `objects` in a single object
                    "type": "object",
                    "properties": {},
                    "required": [],
                    "additionalProperties": False
                },
                "clock": {  # clock object
                    "type": "object",
                    "properties": {},
                    "required": [],
                    "additionalProperties": True
                },
                "_directives": {  # directives object
                    "type": "object",
                    "properties": {},
                    "required": [],
                    "additionalProperties": True
                },
                "_preamble": {  # preamble object
                    "type": "object",
                    "properties": {},
                    "required": [],
                    "additionalProperties": True
                },
                "classes": {  # classes object
                    "type": "object",
                    "properties": {},
                    "required": [],
                    "additionalProperties": True
                },
                "schedules": {  # schedules object
                    "type": "array",
                    "properties": {},
                    "required": [],
                    "additionalProperties": True
                },
            },
            "definitions": {
                "itemConditionals": {
                    "type": "object",
                    "properties": {
                        "ifdef": {
                        "type": "object",
                        "additionalProperties": {
                            "anyOf": [
                            { "type": "number" },
                            { "type": "string" }
                            ]
                        }
                        },
                        "ifndef": {
                        "type": "object",
                        "additionalProperties": {
                            "anyOf": [
                            { "type": "number" },
                            { "type": "string" }
                            ]
                        }
                        },
                        "ifexist": {
                        "type": "object",
                        "additionalProperties": {
                            "anyOf": [
                            { "type": "number" },
                            { "type": "string" }
                            ]
                        }
                        },
                        "if": {
                        "type": "object",
                        "additionalProperties": {
                            "anyOf": [
                            { "type": "number" },
                            { "type": "string" }
                            ]
                        }
                        },
                    },
                    "additionalProperties": False,
                    "required": []
                },
                "entityConditionals": {
                    "type": "object",
                    "properties": {
                        "ifdef": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        },
                        "ifndef": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        },
                        "ifexist": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        },
                        "if": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        },                            
                        "print": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        },
                        "error": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        },
                        "warning": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        }
                    },
                    "additionalProperties": False
                },        
            },
            "required": [],  # Add required fields dynamically if needed
            "additionalProperties": True  # Allow additional properties globally
        }

        # Add `module_entities` into the `modules` section
        for name in self.module_entities:
            if name == "_legacy":
                continue
            if hasattr(self.module_entities[name], "to_schema"):  # Ensure `to_schema` exists
                module_schema = self.module_entities[name].to_schema()
                if name in ['clock', '_directives', '__preamble', '_legacy', 'classes', 'schedules']:
                    schema["properties"][name] = module_schema
                else:
                    schema["properties"]["modules"]["properties"][name] = module_schema
            else:
                raise AttributeError(f"Entity '{name}' in `module_entities` does not implement `to_schema()`.")

        # Add `object_entities` into the `objects` section
        for name in self.object_entities:
            if hasattr(self.object_entities[name], "to_schema"):  # Ensure `to_schema` exists
                object_schema = self.object_entities[name].to_schema(True)
                schema["properties"]["objects"]["properties"][name] = object_schema
            else:
                raise AttributeError(f"Entity '{name}' in `object_entities` does not implement `to_schema()`.")

        return schema


    def add_object(self, _type, name, params):
        """Add a new object to the model.
        
        Args:
            _type (str): The object type
            name (str): The object name
            params (dict): Object parameters
            
        Returns:
            Entity instance: The created object instance
        """
        # add the new object type to the model
        if _type not in self.model:
            self.model[_type] = {}
        # add name and set object entity instance to model type
        self.model[_type][name] = self.set_object_instance(_type, name, params)
        return self.model[_type][name]

    def _extract_inline_comment(self, line):
        """Extract inline comment from a line.
        
        Args:
            line (str): Line that may contain an inline comment
            
        Returns:
            tuple: (comment_text, line_without_comment)
        """
        comment_pos = line.find("//")
        if comment_pos > 0:
            comment_text = line[comment_pos + 2:].strip()
            line_without_comment = line[:comment_pos].strip()
            return comment_text, line_without_comment
        else:
            return None, line

    def _process_inline_comment(self, line):
        """Process inline comment from a line and extract comment text and tokens.
        
        Args:
            line (str): Line that may contain an inline comment
            
        Returns:
            tuple: (comment_text, tokens) where tokens is the split line
        """
        comment_pos = line.find("//")
        if comment_pos == 0:
            # Line starts with comment
            comment_text = line[comment_pos + 2:].strip()
            return comment_text, []
        elif comment_pos > 0:
            # Line has inline comment
            comment_text = line[comment_pos + 2:].strip()
            tokens = line.split(" ")
            return comment_text, tokens
        else:
            # No comment found
            return None, line.split(" ")

    def glm_schedule(self, line, itr):
        """Parse a schedule definition from GLM lines.
        
        This method extracts schedule definitions from GLM files, handling
        both single-line and multi-line schedule blocks.
        
        Args:
            line (str): The line containing the schedule definition
            itr (iter): Iterator over the list of lines
            
        Returns:
            str: The schedule name/identifier
        """
        # This only grab the lines, real parsing of the schedule
        m_sched = re.search(r'schedule\W+(\w+)\s*([;{])', line, re.IGNORECASE)
        if m_sched:
            # schedule found
            self.schedule_types[m_sched.group(1)] = []
            self.schedule_types[m_sched.group(1)].append(line)
            if m_sched.group(2) == '{':
                # multi-line schedule
                oend = 1
                tab = ["  "]
                while oend:
                    line = next(itr)
                    if re.search('}', line):
                        # end of the schedule
                        tab.remove("  ")
                        oend -= 1
                    self.schedule_types[m_sched.group(1)].append(''.join(tab) + line)
                    if re.search('{', line):
                        # start of the sub schedule
                        tab.append("  ")
                        oend += 1
        return m_sched.group(1)

    def _finalize_comments_and_params(self, params, inside_comments, inline_comments):
        """Finalize comment collection and add them to params.
        
        Args:
            params (dict): Parameters dictionary to update
            inside_comments (list): List of inside comments
            inline_comments (dict): Dictionary of inline comments
        """
        # attach any preceding // comments
        if self.outside_comments:
            params['outside_comments'] = self.outside_comments
            self.outside_comments = []
        # attach any preceding #error directives
        if hasattr(self, 'outside_errors') and self.outside_errors:
            params['outside_trigger_error'] = self.outside_errors
            self.outside_errors = []
        # attach any preceding #warning directives
        if hasattr(self, 'outside_warnings') and self.outside_warnings:
            params['outside_trigger_warning'] = self.outside_warnings
            self.outside_warnings = []
        # attach any preceding #print directives
        if hasattr(self, 'outside_prints') and self.outside_prints:
            params['outside_print'] = self.outside_prints
            self.outside_prints = []
        if len(inside_comments) > 0:
            params['inside_comments'] = inside_comments
        if len(inline_comments) > 0:
            params['inline_comments'] = inline_comments

    def glm_module(self, mod, line, itr):
        """Store a clock/module/class in the model structure.

        Args:
            mod (str): GLM type [date, class, module]
            line (str): GLM line containing the object definition
            itr (iter): Iterator over the list of lines
            
        Returns:
            str: The module type
        """

        # Collect parameters
        _type = ""
        params = {}
        class_fields = []
        inside_comments = []
        inline_comments = dict()
        inside_errors = []
        inside_warnings = []
        inside_prints = []
        inside_if_statements = []
        # Set the clock to date module
        if mod in ["date"]:
            line = mod + " " + line

        # Identify the object type
        if line.find(";") > 0:
            # handle single-line module declarations
            comment_text, _ = self._extract_inline_comment(line)
            if comment_text:
                params['inline_comment'] = comment_text
                inline_comments = dict()
            # attach any // comments
            if self.outside_comments:
                params['outside_comments'] = self.outside_comments
                self.outside_comments = []
            # attach any #error directives
            if hasattr(self, 'outside_errors') and self.outside_errors:
                params['outside_trigger_error'] = self.outside_errors
                self.outside_errors = []
            # attach any #warning directives
            if hasattr(self, 'outside_warnings') and self.outside_warnings:
                params['outside_trigger_warning'] = self.outside_warnings
                self.outside_warnings = []
            # attach any #print directives
            if hasattr(self, 'outside_prints') and self.outside_prints:
                params['outside_print'] = self.outside_prints
                self.outside_prints = []
            m = re.search(mod + r' ([^;\s]+)[;\s]', line, re.IGNORECASE)
            _type = m.group(1)
            self.set_module_instance(_type, params)
            return _type

        if line.find("{") > 0:
            m = re.search(mod + r' ([^{\s]+)[{\s]', line, re.IGNORECASE)
            _type = m.group(1)

        comment_text, line_without_comment = self._extract_inline_comment(line)
        if comment_text:
            inline_comments[line_without_comment] = comment_text

        done = False
        line = next(itr).strip()
        while not done:
            # capture module-level #error directives
            if re.search(r'#error\b', line):
                inside_errors.append(self._extract_directive_content(line, 'error'))
                line = next(itr).strip()
                continue
            # capture module-level #warning directives
            if re.search(r'#warning\b', line):
                inside_warnings.append(self._extract_directive_content(line, 'warning'))
                line = next(itr).strip()
                continue
            # capture module-level #print directives
            if re.search(r'#print\b', line):
                inside_prints.append(self._extract_directive_content(line, 'print'))
                line = next(itr).strip()
                continue
            # Process comments and extract inline comments
            comment_text, tokens = self._process_inline_comment(line)
            if comment_text is not None:
                if len(tokens) == 0:  # Line starts with comment
                    inside_comments.append(comment_text)
                    line = ";"
                else:  # Line has inline comment
                    inline_comments[tokens[0]] = comment_text
            # find if/ifdef/ifndef/ifexist directives
            for d in ('ifdef', 'ifndef', 'ifexist', 'if'):
                if re.search(rf'#{d}\b', line):
                    # Extract just the condition part without directive
                    condition = self._extract_directive_content(line, d)
                    inside_if_statements.append({"type": d, "condition": condition})
            if re.search(r'#else\b', line):
                # Convert the else to an ifnot
                if len(inside_if_statements) > 0:
                    condition = inside_if_statements[-1]["condition"]
                    inside_if_statements.pop()
                    inside_if_statements.append({"type": "ifnot", "condition": condition})
            if re.search('#endif', line) and len(inside_if_statements) > 0:
                inside_if_statements.pop()
            # find a parameter
            m = re.match(r'\s*(\S+) ([^;]+);', line)
            if m:
                ptype, pname = m.group(1), m.group(2).strip()
                # record each field for user-defined classes
                if mod == 'class':
                    class_fields.append({'type': ptype, 'name': pname})
                if (inside_if_statements):
                    #check if params[ptype] exists
                    if ptype not in params:
                        params[ptype] = {}
                    self.add_conditionals_to_item(params[ptype], pname, inside_if_statements)
                else:
                    params[ptype] = pname
            
            if re.search('}', line):
                done = 1
            else:
                line = next(itr).strip()

        self._finalize_comments_and_params(params, inside_comments, inline_comments)
        # Attach module-level trigger error
        if inside_errors:
            params['trigger_error'] = inside_errors
        # Attach module-level trigger warning
        if inside_warnings:
            params['trigger_warning'] = inside_warnings
        # Attach module-level print
        if inside_prints:
            params['print'] = inside_prints
        # record class blueprint fields for user-defined classes
        if mod == 'class':
            # use collected class_fields to capture all defined fields
            self.class_definitions[_type] = class_fields
        self.set_module_instance(_type, params)

        return _type

    def _process_object_parameter(self, param, val, name_prefix, insideIfDefs, params, comments, inside_comments, line=None):
        """Process a single object parameter and handle special cases.
        
        Args:
            param (str): Parameter name
            val (str): Parameter value
            name_prefix (str): Prefix for names
            insideIfDefs (list): List of ifdef conditions
            params (dict): Dictionary to store parameters
            comments (list): List of comments
            inside_comments (list): List of inside comments
            line (str, optional): Current line for $ command processing
            
        Returns:
            tuple: (processed_name, updated_line) where processed_name is the name 
                   if param is 'name', otherwise None
        """
        if param == 'name':
            # found a parameter name
            return gld_strict_name(name_prefix + val), line
        elif param == 'object':
            # This case should be handled separately for nested objects
            return None, line
        else:
            # found a parameter val
            if val == "$" and line is not None:
                # found $ command
                pos = line.find("{")
                pos1 = line.find(";")
                val = val + line[pos:pos1]
                line = ""
            
            if param in ["to", "from", "configuration", "parent"]:
                val = gld_strict_name(name_prefix + val)
            
            if len(insideIfDefs) > 0:
                if( param not in params):
                    params[param] = {}
                self.add_conditionals_to_item(params[param], val.strip(), insideIfDefs)
            else:
                params[param] = val.strip()
            
            if len(comments) > 0:
                inside_comments[param] = comments
                comments.clear()
            
            return None, line

    def glm_object(self, parent, line, itr, oidh, counter):
        """Store an object in the model structure.

        Args:
            parent (str): Name of parent object (used for nested object defs)
            line (str): GLM line containing the object definition
            itr (iter): Iterator over the list of lines
            oidh (dict): Hash of object id's to object names
            counter (int): Object counter
            
        Returns:
            tuple: (current_line, counter, name) - the current line, counter, and name
        """
        # Identify the object type and raw id
        oid = ""
        m = re.search(r'object ([^:{\s]+)[:{\s]', line, re.IGNORECASE)
        _type = m.group(1)
        # If the object has an id qualifier (e.g., ..N), store it
        n = re.search(r'object ([^:]+:[^{\s]+)', line, re.IGNORECASE)
        if n:
            oid = n.group(1)
        # single-line object (no body) ends with ';'
        if line.strip().endswith(';') and '{' not in line:
            # capture multiplicity count if present
            params = {}
            qty_match = re.search(r'\.\.(\d+)', oid)
            if qty_match:
                params['object_count'] = int(qty_match.group(1))
            # default name
            counter += 1
            name = f"{_type}_{counter}"
            oidh[name] = name
            self.add_object(_type, name, params)
            return line, counter, name

        # Collect parameters
        counter += 1
        name = None
        name_prefix = ''
        params = {}
        # handle multiplicity syntax (e.g., object house:..28 indicates multiple instances)
        m_qty = re.search(r'\.\.(\d+)$', oid)
        if m_qty:
            params['object_count'] = int(m_qty.group(1))
        # Collect comments
        comments = []
        inside_comments = []
        inline_comments = dict()
        # Collect error, warning, and print directives inside object
        inside_errors = []
        inside_warnings = []
        inside_prints = []
        done = False
        outsideIfDefs = self.ifdef_lines.copy()
        insideIfDefs = []

        pos = line.find("//")
        if pos > 0:
            substring = line[pos + 2:].strip()
            before_comment = line.split("//", 1)[0].strip()
            inline_comments[before_comment] = substring
        self.add_conditionals_to_dict(params, outsideIfDefs)
        line = next(itr)
        if len(parent):
            params['parent'] = parent
        while not done:
            # capture object-level #error directives
            if re.search(r'#error\b', line):
                content = self._extract_directive_content(line, 'error')
                inside_errors.append(content)
                line = next(itr)
                continue
            # capture object-level #warning directives
            if re.search(r'#warning\b', line):
                content = self._extract_directive_content(line, 'warning')
                inside_warnings.append(content)
                line = next(itr)
                continue
            # capture object-level #print directives
            if re.search(r'#print\b', line):
                content = self._extract_directive_content(line, 'print')
                inside_prints.append(content)
                line = next(itr)
                continue
            # Process comments and extract inline comments
            comment_text, tokens = self._process_inline_comment(line)
            if comment_text is not None:
                if len(tokens) == 0:  # Line starts with comment
                    inside_comments.append(comment_text)
                    line = ";"
                else:  # Line has inline comment
                    if tokens[0].lower() != 'object':
                        inline_comments[tokens[0]] = comment_text
            # find conditional directives inside object and record
            for d in ('ifdef', 'ifndef', 'ifexist', 'if'):
                if re.search(rf"#{d}\b", line):
                    condition = self._extract_directive_content(line, d)
                    insideIfDefs.append({"type": d, "condition": condition})
            # handle else as negation of last condition
            if re.search(r'#else\b', line) and len(insideIfDefs) > 0:
                cond = insideIfDefs[-1]["condition"]
                # switch to ifnot
                insideIfDefs.pop()
                insideIfDefs.append({"type": "ifnot", "condition": cond})
                params.setdefault('ifnot', []).append(cond)
            # end conditional block
            if re.search('#endif', line) and len(insideIfDefs) > 0:
                insideIfDefs.pop()
            intobj = 0
            m = re.match(r'\s*(\S+) ([^;{]+)[;{]', line)
            if '${' in line and line.strip().endswith(';'):
                # Split into parameter and value parts
                tokens = line.split(None, 1)
                if len(tokens) > 1:
                    param = tokens[0]
                    # Remove the trailing semicolon from the remainder
                    val = tokens[1].rsplit(";", 1)[0].strip()
                    processed_name, updated_line = self._process_object_parameter(param, val, name_prefix,
                             insideIfDefs, params, comments, inside_comments, line)
            elif m:
                param = m.group(1)
                val = m.group(2)
                if param == 'object':
                    # found a nested object
                    intobj += 1
                    if name is None:
                        raise RuntimeError("nested object defined before parent name")
                    line, counter, lname = self.glm_object(name, line, itr, oidh, counter)
                else:
                    # Process parameter using helper method
                    processed_name, updated_line = self._process_object_parameter(
                        param, val, name_prefix, insideIfDefs, params, comments, inside_comments, line
                    )
                    if processed_name is not None:
                        name = processed_name
                    if updated_line != line:
                        line = updated_line

            if line.count('}') > line.count('{'):
                if intobj:
                    intobj -= 1
                    line = next(itr)
                else:
                    done = True
            else:
                line = next(itr)
        # if undefined, use a default name
        if name is None:
            name = name_prefix + _type + "_" + str(counter)
        oidh[name] = name
        # hash an object identifier to the object name
        if n:
            oidh[oid] = name
        self._finalize_comments_and_params(params, inside_comments, inline_comments)
        # attach object-level trigger error
        if inside_errors:
            params['trigger_error'] = inside_errors
        # attach object-level trigger warning
        if inside_warnings:
            params['trigger_warning'] = inside_warnings
        # attach object-level trigger print
        if inside_prints:
            params['print'] = inside_prints          
        # add the new object type to the model
        self.add_object(_type, name, params)

        return line, counter, name
    
    def add_conditionals_to_item(self, item, value, conditionals):
        for cond in conditionals:
            t = cond["type"]
            if t in ("ifdef", "ifndef", "ifexist", "if"):
                # If the conditionals are not already present, initialize them
                if t not in item:
                    item.setdefault(t, {})
                condition = cond["condition"] # Use value if condition is not specified
                item[t][condition] = value  # Use `cond.get` for safety

    def add_conditionals_to_entity(self, entity, conditionals):
        for cond in conditionals:
            t = cond["type"]
            if t in ("ifdef", "ifndef", "ifexist", "if"):
                entity._conditionals.setdefault(t, []).append(cond["condition"])

    def add_conditionals_to_dict(self, obj, conditionals):
        for cond in conditionals:
            t = cond["type"]
            if t in ("ifdef", "ifndef", "ifexist", "if"):
                # Check if '_conditionals' is a key in the dictionary and initialize if missing
                if "_conditionals" not in obj or not isinstance(obj["_conditionals"], dict):
                    obj["_conditionals"] = {}  # Initialize '_conditionals' as a dictionary
                if t not in obj["_conditionals"]:
                    obj["_conditionals"][t] = []  # Initialize the type-specific conditional list
                obj["_conditionals"][t].append(cond["condition"])  # Safely append condition

    def _classify_line(self, line):
        """Classify a line based on its content and return line type and processed line.
        
        Args:
            line (str): The line to classify
            
        Returns:
            tuple: (line_type, processed_line) where line_type is one of:
                   'comment_set', 'comment_include', 'comment_define', 'comment_other',
                   'set', 'include', 'define', 'clock', 'class', 'module', 'schedule', 
                   'object', 'ifdef', 'endif', 'unknown'
        """
        if re.match('^//', line):
            if re.search('#set', line):
                return 'comment_set', line
            elif re.search('#include', line):
                return 'comment_include', line
            elif re.search('#define', line):
                return 'comment_define', line
            elif re.search('#undef', line):
                return 'comment_undef', line
            elif re.search('#error', line):
                return 'comment_error', line
            else:
                return 'comment_other', line
        elif re.search('#set', line):
            return 'set', line
        elif re.search('#include', line):
            return 'include', line
        elif re.search('#define', line):
            return 'define', line
        elif re.search('#undef', line):
            return 'undef', line
        elif re.search('#error', line):
            return 'error', line
        elif re.search('clock', line):
            return 'clock', line
        elif re.search('class', line):
            return 'class', line
        elif re.search('module', line):
            return 'module', line
        elif re.search('schedule', line):
            return 'schedule', line
        elif re.search('object', line):
            return 'object', line
        elif re.search('#ifdef', line):
            return 'ifdef', line
        elif re.search('#ifndef', line):
            return 'ifndef', line
        elif re.search('#ifexist', line):
            return 'ifexist', line
        elif re.search(r'#if\b', line):
            return 'if', line
        elif re.search('#else', line):
            return 'else', line
        elif re.search('#endif', line):
            return 'endif', line
        else:
            return 'unknown', line

    def _read_file_lines(self, filename):
        """Read and preprocess lines from a file.
        
        Args:
            filename (str): Path to the file to read
            
        Returns:
            list: List of processed lines (stripped and non-empty)
        """
        lines = []
        with open(filename, 'r') as file:
            line = file.readline()
            while line:
                line = line.replace("\t", " ")
                # Skip whitespace-only lines
                while re.match(r'\s+$', line):
                    line = file.readline()
                    if not line:  # End of file
                        break
                if line:  # Check if we still have a line after skipping whitespace
                    line = line.strip()
                    if len(line) > 0:
                        lines.append(line)
                line = file.readline()
        return lines

    def read_model(self, filename):
        """Read and parse a GLM model file.

        Args:
            filename (str): Full path to the GLM file to read

        Returns:
            bool: True if the model was read successfully, False otherwise
            
        Raises:
            FileNotFoundError: If the specified file doesn't exist
        """
        name = ""
        counter = 0
        h = {}  # OID hash
        lines = []
        self.model = {}
        self.set_lines = []
        self.define_lines = []
        self.include_lines = []
        self.ifdef_lines = []
        self.schedule_types = {}
        self.outside_comments = []
        self.outside_warnings = []
        self.outside_prints = []
        self.outside_setenvs = []
        self.outside_binpaths = []
        self.outside_libpaths = []
        self.outside_incpaths = []
        self.outside_options = []
        self.outside_systems = []
        self.outside_errors = []
        if os.path.isfile(filename):
            lines = self._read_file_lines(filename)
            
            # New preamble processing:
            preamble_lines = []
            while lines and lines[0].startswith("//"):
                preamble_lines.append(lines.pop(0))
            # Save the preamble as its own object in the model
            self.module_entities['__preamble'].comments = preamble_lines
            
            itr = iter(lines)

            for line in itr:
                line_type, processed_line = self._classify_line(line)
                if line_type == 'comment_set' or line_type == 'set':
                    self.set_lines.append(self._extract_directive_content(processed_line, 'set'))
                elif line_type == 'comment_include' or line_type == 'include':
                    self.include_lines.append(self._extract_directive_content(processed_line, 'include'))
                elif line_type == 'comment_define' or line_type == 'define':
                    self.define_lines.append(self._extract_directive_content(processed_line, 'define'))
                elif line_type == 'comment_undef' or line_type == 'undef':
                    self.undef_lines.append(self._extract_directive_content(processed_line, 'undef'))
                elif line_type == 'comment_error' or line_type == 'error':
                    content = self._extract_directive_content(processed_line, 'error')
                    self.error_lines.append(content)
                    self.outside_errors.append(content)
                elif re.search('#warning', processed_line):
                    content = self._extract_directive_content(processed_line, 'warning')
                    self.warning_lines.append(content)
                    self.outside_warnings.append(content)
                elif re.search('#print', processed_line):
                    content = self._extract_directive_content(processed_line, 'print')
                    self.print_lines.append(content)
                    self.outside_prints.append(content)
                elif re.search('#setenv', processed_line):
                    content = self._extract_directive_content(processed_line, 'setenv')
                    self.setenv_lines.append(content)
                    self.outside_setenvs.append(content)
                elif re.search('#binpath', processed_line):
                    content = self._extract_directive_content(processed_line, 'binpath')
                    self.binpath_lines.append(content)
                    self.outside_binpaths.append(content)
                elif re.search('#libpath', processed_line):
                    content = self._extract_directive_content(processed_line, 'libpath')
                    self.libpath_lines.append(content)
                    self.outside_libpaths.append(content)
                elif re.search('#incpath', processed_line):
                    content = self._extract_directive_content(processed_line, 'incpath')
                    self.incpath_lines.append(content)
                    self.outside_incpaths.append(content)
                elif re.search('#option', processed_line):
                    content = self._extract_directive_content(processed_line, 'option')
                    self.option_lines.append(content)
                    self.outside_options.append(content)
                elif re.search('#system', processed_line):
                    content = self._extract_directive_content(processed_line, 'system')
                    self.system_lines.append(content)
                    self.outside_systems.append(content)
                elif re.search('#start', processed_line):
                    content = self._extract_directive_content(processed_line, 'start')
                    self.start_lines.append(content)
                    self.outside_starts.append(content)
                elif line_type == 'clock':
                    name = self.glm_module("date", line, itr)
                elif line_type == 'class':
                    name = self.glm_module("class", line, itr)
                    # Record encountered class types for JSON separation
                    if name and name not in self.class_types:
                        self.class_types.append(name)
                elif line_type == 'module':
                    name = self.glm_module("module", line, itr)
                    # attach any conditional directives to the module entity
                    entity = self.module_entities[name]
                    self.add_conditionals_to_entity(entity, self.ifdef_lines)
                elif line_type == 'schedule':
                    name = self.glm_schedule(line, itr)
                elif line_type == 'object':
                    line, counter, name = self.glm_object("", line, itr, h, counter)
                elif line_type in ('ifdef', 'ifndef', 'ifexist', 'if'):
                    # record entering a conditional directive
                    self.ifdef_lines.append({
                        "type": line_type,
                        "condition": self._extract_directive_content(line, line_type)
                    })
                elif line_type == 'else':
                    # Convert else to ifnot
                    if len(self.ifdef_lines) > 0:
                        condition = self.ifdef_lines[-1]["condition"]
                        self.ifdef_lines.pop()
                        self.ifdef_lines.append({"type": "ifnot", "condition": condition})
                elif line_type == 'endif':
                    self.ifdef_lines.pop()
                elif line_type == 'comment_other':
                    self.outside_comments.append(processed_line)
                else:
                    print('Un-parsed line "' + line + '"')

            # Put directives into separate object
            self.module_entities['_directives'].sets = self.set_lines
            self.module_entities['_directives'].includes = self.include_lines
            self.module_entities['_directives'].defines = self.define_lines
            self.module_entities['_directives'].undefs = self.undef_lines
            # extended directives
            self.module_entities['_legacy'].setenvs = self.setenv_lines
            self.module_entities['_legacy'].binpaths = self.binpath_lines
            self.module_entities['_legacy'].libpaths = self.libpath_lines
            self.module_entities['_legacy'].incpaths = self.incpath_lines
            self.module_entities['_legacy'].options = self.option_lines
            self.module_entities['_legacy'].systems = self.system_lines
            self.module_entities['_legacy'].starts = self.start_lines
            self.hash = h
            return True
        else:
            raise FileNotFoundError(f"{filename} not found")

    @staticmethod
    def _extract_directive_content(line, directive):
        """Extract the content part of a directive line.
        
        Removes the directive prefix and returns just the content.
        
        Args:
            line (str): The line containing the directive
            directive (str): The directive name (e.g., 'set', 'include', 'define', 'ifdef')
            
        Returns:
            str: The content without the directive prefix, or the original line if no match
        """
        # Handle commented directives like "// #set ..."
        if line.strip().startswith('//'):
            # For commented lines, keep the comment prefix but extract directive content
            comment_match = re.search(rf'//\s*#{directive}\s+(.*)', line)
            if comment_match:
                return f"// {comment_match.group(1).strip()}"
        else:
            # For regular directives, extract just the content
            directive_match = re.search(rf'#{directive}\s+(.*)', line)
            if directive_match:
                return directive_match.group(1).strip()
        
        # If no match found, return original line
        return line

