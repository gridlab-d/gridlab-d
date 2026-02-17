"""
GLM parsing logic and model management.

Parses GridLAB-D model files (.glm) into structured data representations and
handles conversion to JSON format via the GLMModel class.

Supported: Objects, modules, directives (#include, #define, #set, #undef),
legacy directives (#setenv, #binpath, etc.), schedules, classes, clock, comments.

Unsupported: Conditional directives (#ifdef, #ifndef, #if, #else, #endif) will
raise GLMConditionalError and must be resolved before conversion.
"""

import os
import re
import pyjson5
from importlib_resources import files

# Import the modular components with fallback
try:
    # Try relative imports first (for package usage)
    from .glm_entities import Item, Entity, O_Entity, GLM
    from .glm_utils import gld_strict_name, add_attr_to_entity, convert_suffix_id
except (ImportError, ValueError):
    # Fall back to direct imports (for standalone usage)
    from glm_entities import Item, Entity, O_Entity, GLM
    from glm_utils import gld_strict_name, add_attr_to_entity, convert_suffix_id

glm_entities_path = files('references').joinpath('glm_classes.json')

def create_conditional_error_message(directive, line, context=""):
    """Create a standardized error message for conditional directives.
    
    Args:
        directive (str): The conditional directive name (e.g., 'ifdef', 'else', 'endif')
        line (str): The line content where the directive was found
        context (str, optional): Additional context like 'inside module/class' or 'inside object'
        
    Returns:
        str: Formatted error message
    """
    context_part = f" {context}" if context else ""
    return (
        f"Conditional directive '#{directive}' found{context_part} at line: '{line.strip()}'. "
        f"Please remove all conditional directives (#ifdef, #ifndef, #ifexist, #if, #else, #endif) "
        f"from the GLM file before conversion to JSON."
    )

class GLMConditionalError(Exception):
    """Raised when conditional directives (#ifdef, #ifndef, #if, #else, #endif) are
    encountered during parsing. These must be resolved before conversion to JSON.
    """
    pass

class GLMModel:
    """Parses and manages GridLAB-D model files.
    
    Orchestrates GLM file parsing, maintains collections of model elements
    (objects, modules, directives, schedules, classes), and converts to JSON.
    
    Main methods: read_model(), entities_to_json()
    """
    
    def __init__(self):
        """Initialize GLMModel and load class definitions from reference JSON."""
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
        self.class_entities = {}
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
            entity.add_attr("TEXTARRAY", "#include", "", "#include", value=[])
            entity.add_attr("TEXTARRAY", "#define", "", "#define", value=[])
            entity.add_attr("TEXTARRAY", "#set", "", "#set", value=[])
            entity.add_attr("TEXTARRAY", "#undef", "", "#undef", value=[])
            self.module_entities["_directives"] = entity
            entity = Entity("_legacy", None)
            entity.add_attr("TEXTARRAY", "#setenv", "", "#setenv", value=[])
            entity.add_attr("TEXTARRAY", "#binpath", "", "#binpath", value=[])
            entity.add_attr("TEXTARRAY", "#libpath", "", "#libpath", value=[])
            entity.add_attr("TEXTARRAY", "#incpath", "", "#incpath", value=[])
            entity.add_attr("TEXTARRAY", "#option", "", "#option", value=[])
            entity.add_attr("TEXTARRAY", "#system", "", "#system", value=[])
            entity.add_attr("TEXTARRAY", "#start", "", "#start", value=[])
            self.module_entities["_legacy"] = entity
            entity = Entity("__preamble", None)
            entity.add_attr("TEXTARRAY", "Preamble comments", "", "comments", value=[])
            self.module_entities["__preamble"] = entity
            inheritance = []
            for module_name in self.classes:
                self.module_types.append(module_name)
                for object_name in self.classes[module_name]:
                    obj = self.classes[module_name][object_name]
                    if obj is None:
                        self.classes[module_name][object_name] = {}
                    if object_name == "global_attributes":
                        entity = Entity(module_name, None)
                        if obj is None:
                            self.module_entities[module_name] = entity
                            continue
                        for attr in obj:
                            attr_name = attr.replace(module_name+"::", "")
                            add_attr_to_entity(entity, attr_name, obj[attr])
                        self.module_entities[module_name] = entity
                    else:
                        obj = self.classes[module_name][object_name]
                        entity = O_Entity(self, object_name, None)
                        entity.add_attr("OBJECT", "Parent", "", "parent", value=None)
                        for attr in obj:
                            if obj[attr]['type'] == "parent":
                                inheritance.append([object_name, attr])
                            else:
                                if obj[attr]['type'] == "complex":
                                    obj[attr]['type'] = "double"
                                add_attr_to_entity(entity, attr, obj[attr])
                        self.object_entities[object_name] = entity
                        setattr(self.glm, object_name, entity)

            # Add inheritance
            # multiple inheritance for powerflow object(s)
            # hard coded as to walk parent/child relationships [node, link, triplex_node]
            for myheritance in inheritance:
                if myheritance[0] in ['node', 'link', 'triplex_node']:
                    entity = self.object_entities[myheritance[0]]
                    inherit = self.object_entities[myheritance[1]]
                    for p_attr, p_item in inherit.__dict__.items():
                        if isinstance(p_item, Item):
                            if not p_attr in ['name','parent','instances']:
                                setattr(entity, p_attr, p_item)

            for myheritance in inheritance:
                # print("Inheritance object ->", myheritance[0], "for", myheritance[1])
                if not myheritance[0] in ['node', 'link', 'triplex_node']:
                    entity = self.object_entities[myheritance[0]]
                    inherit = self.object_entities[myheritance[1]]
                    for p_attr, p_item in inherit.__dict__.items():
                        if isinstance(p_item, Item):
                            if not p_attr in ['name','parent','instances']:
                                setattr(entity, p_attr, p_item)


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
                print(f"Available module_entities keys: {list(self.module_entities.keys())}")
                print(f"Available classes keys: {list(self.classes.keys()) if hasattr(self, 'classes') else 'No classes loaded'}")
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

    def set_class_instance(self, class_type, params):
        """Create and configure a class instance.
        
        Args:
            class_type (str): The class type name
            params (dict): Parameters for the class instance
            
        Returns:
            Entity instance or None: The created class instance
            
        Raises:
            TypeError: If class_type is not a string
        """
        if isinstance(class_type, str):
            try:
                entity = self.class_entities[class_type]
                return entity.set_instance(class_type, params)
            except KeyError:
                # Create new class entity
                self.class_types.append(class_type)
                entity = Entity(class_type, None)
                self.class_entities[class_type] = entity
                for items in params:
                    if items in ["integer", "double", "string"]:
                        entity.add_attr('TEXT', items[0], "", items[0], "")
                return entity.set_instance(class_type, params)
        else:
            raise TypeError(f"{class_type} must be a string and is not.")
        return None

    def set_object_instance(self, obj_type, object_name, params):
        """Create and configure an object instance.
        
        Args:
            obj_type (str): The object type name
            object_name (str): The internal identifier for the object instance (may be UUID-based for unnamed objects)
            params (dict): Parameters for the object instance
            
        Returns:
            Entity instance or None: The created object instance
            
        Raises:
            TypeError: If obj_type is not a string
        """
        if isinstance(obj_type, str):
            instance_name = object_name
            
            try:
                # Try to retrieve the existing entity
                entity = self.object_entities[obj_type]
            except KeyError:
                # Handle unrecognized object types
                print(f"Unrecognized GRIDLABD object and id: {obj_type} {instance_name}, must be a new object")
                if not obj_type in self.class_types:
                    print(f"Unrecognized user class/object and id: "
                          f"{obj_type} {instance_name}")
                # Create a new entity for the unrecognized object
                entity = O_Entity(obj_type, instance_name, None)
                self.object_entities[obj_type] = entity
                # Add all parameters to the new entity
                for param, value in params.items():
                    entity.add_attr('TEXT', param, "", param, value)
            return entity.set_instance(instance_name, params)
        else:
            raise TypeError(f"GRIDLABD object type must be a string and is not.")

    def entities_to_json(self):
        """Convert all parsed entities to JSON-serializable format.
        
        Returns:
            dict: JSON representations of all entities (modules, objects, directives, etc.).
        """
        diction = {}
        for name in self.module_entities:
            # Special handling for __preamble: only include if it has comments
            if name == '__preamble':
                preamble_entity = self.module_entities[name]
                if hasattr(preamble_entity, 'comments') and preamble_entity.comments:
                    comments_value = preamble_entity.comments.value if hasattr(preamble_entity.comments, 'value') else preamble_entity.comments
                    if bool(comments_value) and len(comments_value) > 0:
                        diction[name] = preamble_entity.to_json()
            else:
                value = self.module_entities[name].to_json()
                if value is not None:
                    diction[name] = self.module_entities[name].to_json()
        # Add class entities to the output
        for name in self.class_entities:
            value = self.class_entities[name].to_json()
            if value is not None:
                diction[name] = self.class_entities[name].to_json()
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
            schedules = {}
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
                    inline_comments = []
                for l in lines_blk:
                    stripped_line = l.strip()  # Remove leading and trailing whitespace
                    if stripped_line.startswith('//'):
                        if name != "" and name != stripped_line[2:].strip():
                            block_data = {'name': name, 'items': items}
                            if inline_comments:
                                block_data['inline_comments'] = inline_comments
                            blocks.append(block_data)
                            items = []
                            inline_comments = []
                        name = stripped_line[2:].strip()  # Extract everything after '//' and remove extra spaces
                    elif stripped_line and not stripped_line.startswith('//'): 
                        # Check for inline comment
                        if '//' in stripped_line:
                            entry_part, comment_part = stripped_line.split('//', 1)
                            items.append(entry_part.strip().rstrip(';'))
                            inline_comments.append(comment_part.strip())
                        else:
                            items.append(stripped_line)
                block_data = {'name': name, 'items': items}
                if inline_comments:
                    block_data['inline_comments'] = inline_comments
                blocks.append(block_data)
                schedules[sched_name] = blocks
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
                    "type": "object",
                    "additionalProperties": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "name": {"type": "string"},
                                "items": {
                                    "type": "array",
                                    "items": {"type": "string"}
                                },
                                "inline_comments": {
                                    "type": "array",
                                    "items": {"type": "string"}
                                }
                            },
                            "required": [],
                            "additionalProperties": False
                        }
                    }
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

        # Override _directives schema to match actual JSON output structure
        # (where #set and #define are objects, not arrays)
        schema["properties"]["_directives"] = {
            "type": "object",
            "properties": {
                "_conditionals": {
                    "$ref": "#/definitions/entityConditionals"
                },
                "#include": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "List of included files"
                },
                "#set": {
                    "type": "object",
                    "properties": {"type": "string"},
                    "description": "Global variable assignments"
                },
                "#define": {
                    "type": "object",
                    "properties": {"type": "string"},
                    "description": "Macro definitions"
                },
                "#undef": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Undefined macros"
                }
            },
            "required": [],
            "additionalProperties": False
        }

        # Override __preamble schema to match actual JSON output structure
        schema["properties"]["__preamble"] = {
            "type": "object",
            "properties": {
                "_conditionals": {
                    "$ref": "#/definitions/entityConditionals"
                },
                "comments": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Preamble comments from the GLM file"
                }
            },
            "required": [],
            "additionalProperties": False
        }

        # Add `object_entities` into the `objects` section
        for name in self.object_entities:
            if name == "capacitor":
                pass
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
            name (str or None): The object name (None if no name provided)
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
            comment_text = line[comment_pos:].strip()
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
            comment_text = line[comment_pos:].strip()
            return comment_text, []
        elif comment_pos > 0:
            # Line has inline comment
            comment_text = line[comment_pos:].strip()
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

        m_sched = re.search(r'schedule\W+(\w+)\s*([;{])', line, re.IGNORECASE)
        if m_sched:
            # schedule found
            self.schedule_types[m_sched.group(1)] = []
            self.schedule_types[m_sched.group(1)].append(line)
            if m_sched.group(2) == '{' and not line.strip().startswith("//"):
                # multi-line schedule
                oend = 1
                tab = ["  "]
                while oend:
                    line = next(itr)
                    if '{' in line and not line.strip().startswith("//"):
                        # If there's a name before the '{', pick it up as a separate line
                        before_brace = line.split('{', 1)[0].strip()  # Extract text before '{'
                        if before_brace:  # Check if there's any text before '{'
                            # Adding comments because that's what indicates the schedule's name
                            self.schedule_types[m_sched.group(1)].append(''.join(tab) + '// ' + before_brace)
                        # Count the occurrences of '{'
                        count_opening_brackets = line.count('{')
                        # Optional brackets surrounding the schedule data
                        oend += count_opening_brackets
                        continue
                    
                    if re.search('}', line) and not line.strip().startswith("//"):
                        count_closing_brackets = line.count('}')
                        oend -= count_closing_brackets
                        if oend > 0:
                            # schedule still going
                            continue
                        tab.remove("  ")
                    self.schedule_types[m_sched.group(1)].append(''.join(tab) + line)
                if re.search(r'{\s*$', line) and not line.strip().startswith("//"):
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
        # Set the clock to date module
        if mod in ["date"]:
            line = mod + " " + line

        # Identify the object type
        if line.find(";") > 0:
            # handle single-line module declarations
            comment_text, _ = self._extract_inline_comment(line)
            if comment_text:
                params['inline_comments'] = comment_text
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
            
            # Check if this is a single-line module with parameters in braces
            if "{" in line and "}" in line:
                # Parse: module powerflow { solver_method NR; };
                match = re.search(mod + r'\s+([^{\s]+)\s*\{\s*([^}]*)\s*\}', line, re.IGNORECASE)
                if match:
                    _type = match.group(1)
                    param_content = match.group(2).strip()
                    
                    # Parse parameters within the braces
                    if param_content:
                        # Split by semicolon and parse each parameter
                        param_parts = [p.strip() for p in param_content.split(';') if p.strip()]
                        for param_part in param_parts:
                            # Split parameter into name and value
                            if ' ' in param_part:
                                tokens = param_part.split(None, 1)
                                param_name = tokens[0]
                                param_value = tokens[1] if len(tokens) > 1 else ""
                                # Check if value is a reference to an object with name:id
                                param_value = convert_suffix_id(param_value)
                                params[param_name] = param_value
                    
                    self.set_module_instance(_type, params)
                    return _type
            else:
                # Simple single-line module: module powerflow;
                match = re.search(mod + r' ([^;\s]+)[;\s]', line, re.IGNORECASE)
                _type = match.group(1)
                self.set_module_instance(_type, params)
                return _type

        if "{" in line:
            # Match the module name before the opening brace
            m = re.search(mod + r'\s+([^{\s]+)', line, re.IGNORECASE)
            _type = m.group(1)
        else:
            # Continue reading lines until '{' is found
            while "{" not in line:
                m = re.search(mod + r'\s+([^\s]+)', line, re.IGNORECASE)
                if m:
                    _type = m.group(1)  # Capture the module name
                line = next(itr).strip()  # Read the next line and strip whitespace

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
            # Check for conditional directives and throw error
            for d in ('ifdef', 'ifndef', 'ifexist', 'if', 'else', 'endif'):
                if re.search(rf'#{d}\b', line):
                    raise GLMConditionalError(
                        create_conditional_error_message(d, line, "inside module/class")
                    )
            # find a parameter
            m = re.match(r'\s*(\S+) ([^;]+);', line)
            if m:
                ptype, pname = m.group(1), m.group(2).strip()
                # record each field for user-defined classes
                if mod == 'class':
                    class_fields.append({'type': ptype, 'name': pname})
                # Apply gld_strict_name to handle object references
                pname = convert_suffix_id(pname)
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
            self.set_class_instance(_type, params)
        else:
            self.set_module_instance(_type, params)

        return _type

    def _process_object_parameter(self, param, val, name_prefix, params, comments, inside_comments, line=None, name=None):
        """Process a single object parameter and handle special cases.
        
        Args:
            param (str): Parameter name
            val (str): Parameter value
            name_prefix (str): Prefix for names
            params (dict): Dictionary to store parameters
            comments (list): List of comments
            inside_comments (list): List of inside comments
            line (str, optional): Current line for $ command processing
            
        Returns:
            tuple: (processed_name, updated_line) where processed_name is the name 
                   if param is 'name', otherwise None
        """
        if param == 'name':
            # found a parameter name - always use it (don't check if name is None)
            # Remove quotes around the string
            if isinstance(val, str) and val.startswith('"') and val.endswith('"'):
                val = val[1:-1]
            name = val
            # Remove auto-generated flag since we have an explicit name
            params.pop('_auto_generated_name', None)
            return name, line
        elif param == 'object':
            # This case should be handled separately for nested objects
            return None, line
        else:
            # Found a parameter val
            # Remove quotes around the string
            if isinstance(val, str) and val.startswith('"') and val.endswith('"'):
                val = val[1:-1]
                
            if val == "$" and line is not None:
                # found $ command
                pos = line.find("{")
                pos1 = line.find(";")
                val = val + line[pos:pos1]
                line = ""

            params[param] = val

            if len(comments) > 0:
                inside_comments[param] = comments
                comments.clear()
            
            return None, line

    def _get_parent_value_for_nested_object(self, params, name):
        """Determine the parent value for a nested object, using OID if appropriate."""
        obj_decl = params.get('object_declaration', '')
        is_oid = (
            isinstance(obj_decl, str)
            and ':' in obj_decl
            and ' ' not in obj_decl
            and not obj_decl.split(':', 1)[1].startswith('..')
        )
        if params.get('_auto_generated_name', False) and is_oid:
            return obj_decl
        else:
            return name

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
        m = re.search(r'object ([^:{\s]+)(?=[:{\s]|$)', line, re.IGNORECASE)
        _type = m.group(1)
        # If the object has an id qualifier (e.g., ..N), store it
        n = re.search(r'object ([^:]+:[^{\s]+)', line, re.IGNORECASE)
        if n:
            oid = n.group(1)
        # single-line object (no body) ends with ';'
        if line.strip().endswith(';') and '{' not in line:
            params = {}
            # If oid is not empty, save the full object declaration
            if oid:
                params['object_declaration'] = oid
            counter += 1
            # Generate internal tracking name (without parent prefix)
            name = _type + "_" + str(counter)
            params['_auto_generated_name'] = True
            self.add_object(_type, name, params)
            return line, counter, name

        # Collect parameters
        counter += 1
        name_prefix = parent if parent else ''
        # Generate internal tracking name initially (without parent prefix)
        name = _type + "_" + str(counter)
        params = {}
        params['_auto_generated_name'] = True
        
        # If oid is not empty, save the full object declaration
        if oid:
            params['object_declaration'] = oid
        
        # Collect comments
        comments = []
        inside_comments = []
        inline_comments = dict()
        # Collect error, warning, and print directives inside object
        inside_errors = []
        inside_warnings = []
        inside_prints = []
        done = False

        pos = line.find("//")
        if pos > 0:
            substring = line[pos + 2:].strip()
            before_comment = line.split("//", 1)[0].strip()
            inline_comments[before_comment] = substring
        line = next(itr)
        if parent:
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
            # Check for conditional directives inside object and throw error
            for d in ('ifdef', 'ifndef', 'ifexist', 'if', 'else', 'endif'):
                if re.search(rf"#{d}\b", line):
                    raise GLMConditionalError(
                        create_conditional_error_message(d, line, "inside object")
                    )
            intobj = 0
            # Try to match parameter with quoted value first (handles values with semicolons inside quotes)
            m_quoted = re.match(r'\s*(\S+)\s+"([^"]+)"\s*;', line)
            # Then try regular parameter matching
            m = re.match(r'\s*(\S+) ([^;{]+)[;{]', line)
            if '${' in line and line.strip().endswith(';'):
                # Split into parameter and value parts
                tokens = line.split(None, 1)
                if len(tokens) > 1:
                    param = tokens[0]
                    # Remove the trailing semicolon from the remainder
                    val = tokens[1].rsplit(";", 1)[0].strip()
                    processed_name, updated_line = self._process_object_parameter(param, val, name_prefix,
                             params, comments, inside_comments, line, name)
            elif m_quoted:
                # Handle quoted values (which may contain semicolons)
                param = m_quoted.group(1)
                val = '"' + m_quoted.group(2) + '"'
                if param == 'object':
                    # found a nested object
                    intobj += 1
                    parent_value = self._get_parent_value_for_nested_object(params, name)
                    line, counter, lname = self.glm_object(parent_value, line, itr, oidh, counter)
                else:
                    # Process parameter using helper method
                    processed_name, updated_line = self._process_object_parameter(
                        param, val, name_prefix, params, comments, inside_comments, line, name
                    )
                    if processed_name is not None:
                        name = processed_name
                    if updated_line != line:
                        line = updated_line
            elif m:
                param = m.group(1)
                val = m.group(2)
                if param == 'object':
                    # found a nested object
                    intobj += 1
                    parent_value = self._get_parent_value_for_nested_object(params, name)
                    line, counter, lname = self.glm_object(parent_value, line, itr, oidh, counter)
                else:
                    # Process parameter using helper method
                    processed_name, updated_line = self._process_object_parameter(
                        param, val, name_prefix, params, comments, inside_comments, line, name
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
        # Don't generate a name if not provided
        if name is not None:
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
    
    def _classify_line(self, line):
        """Classify a line based on its content and return line type and processed line.
        
        Args:
            line (str): The line to classify
            
        Returns:
            tuple: (line_type, processed_line) where line_type is one of:
                   'comment_other', 'set', 'include', 'define', 'clock', 'class', 'module', 'schedule', 
                   'object', 'ifdef', 'endif', 'unknown'
        """
        if re.match('^//', line):
            return 'comment_other', line
        elif re.search('#set', line) and not re.search('#setenv', line):
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
        elif re.search('extern', line):
            return 'extern', line         
        elif re.search('intrinsic', line):
            return 'intrinsic', line      
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
        with open(filename, 'r', encoding='utf-8', errors='ignore') as file:
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

    def _finalize_preamble(self, preamble_comments):
        """Finalize preamble comments by adding them to module_entities.__preamble"""
        if preamble_comments:
            # Use the existing __preamble entity from module_entities
            preamble_entity = self.module_entities.get('__preamble')
            if preamble_entity and hasattr(preamble_entity, 'comments'):
                # Set the value of the Item object
                preamble_entity.comments.value = preamble_comments[:]
        # If no preamble_comments, leave the entity unchanged (it will return None from to_json())



    def _check_and_finalize_preamble(self, line_type, first_module_or_object_found, preamble_comments):
        """Check if this is the first module/object and finalize preamble if needed"""
        module_object_types = {'module', 'schedule', 'object'}
        if line_type in module_object_types and not first_module_or_object_found:
            self._finalize_preamble(preamble_comments)
            return True
        return first_module_or_object_found

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
            
            # Track whether we've encountered the first module/object
            first_module_or_object_found = False
            preamble_comments = []
            
            itr = iter(lines)

            for line in itr:
                line_type, processed_line = self._classify_line(line)
                
                # Check if this is the first module/object and finalize preamble if needed
                first_module_or_object_found = self._check_and_finalize_preamble(line_type, first_module_or_object_found, preamble_comments)
                if line_type == 'set':
                    content, comment = self._extract_directive_content(processed_line, 'set')
                    self.set_lines.append(content)
                    if comment and not first_module_or_object_found:
                        # Prefix comment with the directive and setting name (inline comment format)
                        setting_name = content.split('=')[0] if '=' in content else content
                        preamble_comments.append(f"#set {setting_name} {comment}")
                elif line_type == 'include':
                    self.include_lines.append(self._extract_directive_content(processed_line, 'include'))
                elif line_type == 'define':
                    content, comment = self._extract_directive_content(processed_line, 'define')
                    self.define_lines.append(content)
                    if comment and not first_module_or_object_found:
                        # Prefix comment with the directive and definition name (inline comment format)
                        define_name = content.split('=')[0] if '=' in content else content
                        preamble_comments.append(f"#define {define_name} {comment}")
                elif line_type == 'undef':
                    self.undef_lines.append(self._extract_directive_content(processed_line, 'undef'))
                elif line_type == 'error':
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
                elif line_type == 'schedule':
                    name = self.glm_schedule(line, itr)
                elif line_type == 'object':
                    line, counter, name = self.glm_object("", line, itr, h, counter)
                elif line_type in ('ifdef', 'ifndef', 'ifexist', 'if', 'else', 'endif'):
                    # Throw error for any conditional directive
                    raise GLMConditionalError(
                        create_conditional_error_message(line_type, processed_line, "in GLM file")
                    )
                elif line_type == 'comment_other':
                    if first_module_or_object_found:
                        self.outside_comments.append(processed_line)
                    else:
                        preamble_comments.append(processed_line)
                elif line_type == 'intrinsic':
                    print(f"Skipping inline code block: {line.strip()}")
                    # Initialize the brace count with the current line
                    brace_count = line.count("{") - line.count("}")
                    # If no opening brace found, check next lines until found
                    while brace_count == 0:
                        try:
                            next_line = next(itr)
                        except StopIteration:
                            break
                        brace_count += next_line.count("{") - next_line.count("}")
                        line = next_line
                    # Continue advancing until all opened braces are closed
                    while brace_count > 0:
                        try:
                            line = next(itr)
                        except StopIteration:
                            break
                        brace_count += line.count("{") - line.count("}")
                else:
                    print('Un-parsed line "' + line + '" with line_type ' + line_type)

            def append_lines(module_entities, entity_key, lines_dict):
                """
                Append values to attributes of the specified entity in module_entities.

                Args:
                    module_entities (dict): The dictionary containing module entities.
                    entity_key (str): The key of the entity in module_entities.
                    lines_dict (dict): A dictionary mapping attribute names to the lines to append.
                """
                entity = module_entities[entity_key]
                for attr, lines in lines_dict.items():
                    setattr(entity, attr, lines)

            # Define the lines to append for '_directives'
            directives_lines = {
                "#set": {},
                "#include": [],
                "#define": {},
                "#undef": []
            }

            for line in self.set_lines:
                try:
                    if "=" in line:
                        key, value = line.split("=", 1)
                        directives_lines["#set"][key.strip()] = value.strip().replace('\\"', '').strip('"')
                    else:
                        print(f"Failed to parse #set line: {line}")
                except Exception as e:
                    print(f"Failed to parse #set line: {line} - {e}")

            converted_includes = []
            for line in self.include_lines:
                try:
                    # Remove quotes, semicolons, and whitespace from the include path
                    include_path = line.strip().rstrip(';').replace('\\"', '').strip('"').strip("'")
                    # Convert .glm extensions to .json
                    if include_path.endswith('.glm'):
                        original_path = include_path
                        include_path = include_path[:-4] + '.json'
                        converted_includes.append((original_path, include_path))
                    directives_lines["#include"].append(include_path)
                except Exception as e:
                    print(f"Failed to parse #include line: {line} - {e}")
            
            # Inform user about converted include files
            if converted_includes:
                print(f"\n📝 Renamed{len(converted_includes)} include file(s) from .glm to .json:")
                for orig, new in converted_includes:
                    print(f"   {orig} → {new}")
                print("⚠️  Please ensure these files have been converted to JSON format.\n")

            for line in self.define_lines:
                try:
                    if "=" in line:
                        key, value = line.split("=", 1)
                        directives_lines["#define"][key.strip()] = value.strip().replace('\\"', '').strip('"')
                    else:
                        print(f"Failed to parse #define line: {line}")
                except Exception as e:
                    print(f"Failed to parse #define line: {line} - {e}")

            for line in self.undef_lines:
                try:
                    directives_lines["#undef"].append(line.strip().replace('\\"', '').strip('"'))
                except Exception as e:
                    print(f"Failed to parse #undef line: {line} - {e}")

            # Define the lines to append for '_legacy'
            legacy_lines = {
                "#setenv": self.setenv_lines,
                "#binpath": self.binpath_lines,
                "#libpath": self.libpath_lines,
                "#incpath": self.incpath_lines,
                "#option": self.option_lines,
                "#system": self.system_lines,
                "#start": self.start_lines,
            }

            # Append lines to '_directives'
            append_lines(self.module_entities, "_directives", directives_lines)

            # Append lines to '_legacy'
            append_lines(self.module_entities, "_legacy", legacy_lines)
                         
            self.hash = h
            return True
        else:
            raise FileNotFoundError(f"{filename} not found")

    @staticmethod
    def _extract_directive_content(line, directive):
        """Extract the content part of a directive line.
        
        Removes the directive prefix and returns just the content.
        For #set and #define directives, also removes inline comments.
        
        Args:
            line (str): The line containing the directive
            directive (str): The directive name (e.g., 'set', 'include', 'define', 'ifdef')
            
        Returns:
            str or tuple: For most directives, returns the content without the directive prefix.
                         For 'set' and 'define', returns tuple (content, comment) where comment
                         is None if no inline comment exists.
        """
        directive_match = re.search(rf'#{directive}\s+(.*)', line)
        if directive_match:
            content = directive_match.group(1).strip()
            
            # For #set and #define, extract and separate inline comments
            if directive in ['set', 'define']:
                # Find '//' that is NOT part of a URL (i.e., not preceded by 'http:' or 'https:')
                # Look for '//' that has whitespace before it (likely a comment)
                comment_match = re.search(r'\s+(//.*)', content)
                if comment_match:
                    comment_text = comment_match.group(1).strip()
                    content_without_comment = content[:comment_match.start()].strip()
                    return (content_without_comment, comment_text)
                else:
                    return (content, None)
            
            return content
        
        # If no match found, return original line
        if directive in ['set', 'define']:
            return (line, None)
        return line


