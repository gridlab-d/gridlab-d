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
    
    Attributes:
        hash (dict): Hash of object identifiers to names
        root: Root node reference
        in_file (str): Input file path
        out_file (str): Output file path  
        model (dict): Parsed model data
        glm (GLM): GLM namespace object
        conn: Connection reference
        modules: Module references
        objects: Object references
        schedule_types (dict): Schedule definitions
        module_types (list): List of module type names
        class_types (list): List of class type names
        module_entities (dict): Module entity definitions
        object_entities (dict): Object entity definitions
        set_lines (list): #set directive lines
        define_lines (list): #define directive lines
        include_lines (list): #include directive lines
        ifdef_lines (list): #ifdef directive lines
        configuration (dict): Configuration settings
        inside_comments (dict): Comments found inside blocks
        outside_comments (dict): Comments found outside blocks
        inline_comments (dict): Inline comments
        classes (dict): Loaded class definitions from JSON
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
        self.include_lines = []
        self.ifdef_lines = []
        self.configuration = {}
        self.inside_comments = dict()
        self.outside_comments = dict()
        self.inline_comments = dict()
        
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
            self.module_entities["_directives"] = entity
            for module_name in self.classes:
                self.module_types.append(module_name)
                for object_name in self.classes[module_name]:
                    if object_name == "global_attributes":
                        obj = self.classes[module_name][object_name]
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
        return diction

    def entities_to_schema(self):
        """Convert all entities to schema format.
        
        Returns:
            dict: Dictionary containing schema representations of all entities
        """
        diction = {}
        for name in self.module_entities:
            diction[name] = self.module_entities[name].to_schema()
        for name in self.object_entities:
            diction[name] = self.object_entities[name].to_schema()
        return diction

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
        if len(self.outside_comments) > 0:
            params['outside_comments'] = self.outside_comments
            self.outside_comments = []
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
        # Collect comments
        inside_comments = []
        inline_comments = dict()
        insideIfDefs = []
        # Set the clock to date module
        if mod in ["date"]:
            line = mod + " " + line

        # Identify the object type
        if line.find(";") > 0:
            # Extract inline comment before returning
            comment_text, _ = self._extract_inline_comment(line)
            if comment_text:
                params['inline_comment'] = comment_text
                inline_comments = dict() 
            if len(self.outside_comments) > 0:
                params['outside_comments'] = self.outside_comments
                self.outside_comments = []      
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
            # Process comments and extract inline comments
            comment_text, tokens = self._process_inline_comment(line)
            if comment_text is not None:
                if len(tokens) == 0:  # Line starts with comment
                    inside_comments.append(comment_text)
                    line = ";"
                else:  # Line has inline comment
                    inline_comments[tokens[0]] = comment_text
            # find if defs
            if re.search('#ifdef', line):
                # Extract just the condition part without #ifdef
                condition = self._extract_directive_content(line, 'ifdef')
                insideIfDefs.append(condition)
            if re.search('#endif', line) and len(insideIfDefs) > 0:
                insideIfDefs.pop()
            # find a parameter
            m = re.match(r'\s*(\S+) ([^;]+);', line)
            if m:
                if len(insideIfDefs) > 0:
                    param = m.group(1)
                    val = m.group(2)
                    if param not in params:
                        params[param] = {"ifDef": {}}
                    localIfDef = insideIfDefs[-1]
                    params[param]["ifDef"][localIfDef] = val.strip()
                else:
                    params[m.group(1)] = m.group(2)

            if re.search('}', line):
                done = 1
            else:
                line = next(itr).strip()

        self._finalize_comments_and_params(params, inside_comments, inline_comments)

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
                if param not in params:
                    params[param] = {"ifDef": {}}
                localIfDef = insideIfDefs[-1]
                params[param]["ifDef"][localIfDef] = val.strip()
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
        # Identify the object type
        oid = ""
        m = re.search(r'object ([^:{\s]+)[:{\s]', line, re.IGNORECASE)
        _type = m.group(1)
        # If the object has an id number, store it
        n = re.search(r'object ([^:]+:[^{\s]+)', line, re.IGNORECASE)
        if n:
            oid = n.group(1)

        # Collect parameters
        counter += 1
        name = None
        name_prefix = ''
        params = {}
        # Collect comments
        comments = []
        inside_comments = []
        inline_comments = dict()
        done = False
        outsideIfDefs = self.ifdef_lines.copy()
        insideIfDefs = []

        pos = line.find("//")
        if pos > 0:
            substring = line[pos + 2:].strip()
            before_comment = line.split("//", 1)[0].strip()
            inline_comments[before_comment] = substring
        if len(self.ifdef_lines) > 0:
            params['ifDef'] = outsideIfDefs
        line = next(itr)
        if len(parent):
            params['parent'] = parent
        while not done:
            # Process comments and extract inline comments
            comment_text, tokens = self._process_inline_comment(line)
            if comment_text is not None:
                if len(tokens) == 0:  # Line starts with comment
                    inside_comments.append(comment_text)
                    line = ";"
                else:  # Line has inline comment
                    if tokens[0].lower() != 'object':
                        inline_comments[tokens[0]] = comment_text
            # find if defs
            if re.search('#ifdef', line):
                # Extract just the condition part without #ifdef
                condition = self._extract_directive_content(line, 'ifdef')
                insideIfDefs.append(condition)
            if re.search('#endif', line) and len(insideIfDefs) > 0:
                insideIfDefs.pop()
            intobj = 0
            m = re.match(r'\s*(\S+) ([^;{]+)[;{]', line)
            if m:
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

            if re.search('}', line):
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
        # add the new object type to the model
        self.add_object(_type, name, params)

        return line, counter, name
    
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
            else:
                return 'comment_other', line
        elif re.search('#set', line):
            return 'set', line
        elif re.search('#include', line):
            return 'include', line
        elif re.search('#define', line):
            return 'define', line
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
        self.outside_comments = []
        if os.path.isfile(filename):
            lines = self._read_file_lines(filename)

            itr = iter(lines)

            for line in itr:
                line_type, processed_line = self._classify_line(line)
                if line_type == 'comment_set' or line_type == 'set':
                    self.set_lines.append(self._extract_directive_content(processed_line, 'set'))
                elif line_type == 'comment_include' or line_type == 'include':
                    self.include_lines.append(self._extract_directive_content(processed_line, 'include'))
                elif line_type == 'comment_define' or line_type == 'define':
                    self.define_lines.append(self._extract_directive_content(processed_line, 'define'))
                elif line_type == 'clock':
                    name = self.glm_module("date", line, itr)
                elif line_type == 'class':
                    name = self.glm_module("class", line, itr)
                    # Record encountered class types for JSON separation
                    if name and name not in self.class_types:
                        self.class_types.append(name)
                elif line_type == 'module':
                    name = self.glm_module("module", line, itr)
                    if len(self.ifdef_lines) > 0:
                        self.module_entities[name].ifDef = self.ifdef_lines
                elif line_type == 'schedule':
                    name = self.glm_schedule(line, itr)
                elif line_type == 'object':
                    line, counter, name = self.glm_object("", line, itr, h, counter)
                elif line_type == 'ifdef':
                    self.ifdef_lines.append(self._extract_directive_content(line, 'ifdef'))
                elif line_type == 'endif':
                    self.ifdef_lines = []
                elif line_type == 'comment_other':
                    self.outside_comments.append(processed_line)
                else:
                    print('Un-parsed line "' + line + '"')

            # Put directives into separate object
            self.module_entities['_directives'].sets = self.set_lines
            self.module_entities['_directives'].defines = self.define_lines
            self.module_entities['_directives'].includes = self.include_lines
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
