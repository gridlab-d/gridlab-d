#!/usr/bin/env python3
"""
JSON to C++ Class Generator
Analyzes a JSON structure and generates corresponding C++ classes
"""

import json
import sys
from typing import Dict, Any, Set

class CppClassGenerator:
    def __init__(self):
        self.classes: Dict[str, Dict] = {}
        self.includes: Set[str] = set()
        
    def analyze_json(self, data: Dict[str, Any], class_name: str = "RootClass"):
        """Analyze JSON structure and extract class definitions"""
        class_def = {
            'name': class_name,
            'members': {},
            'nested_classes': []
        }
        
        for key, value in data.items():
            if isinstance(value, dict):
                # Nested object - create a new class
                nested_class_name = self._to_class_name(key)
                nested_class = self.analyze_json(value, nested_class_name)
                class_def['nested_classes'].append(nested_class_name)
                class_def['members'][key] = f"{nested_class_name}"
                self.includes.add("<memory>")
                
            elif isinstance(value, list):
                if value and isinstance(value[0], dict):
                    # Array of objects
                    item_class_name = f"{self._to_class_name(key)}Item"
                    if value:  # Analyze first item as template
                        self.analyze_json(value[0], item_class_name)
                    class_def['members'][key] = f"std::vector<{item_class_name}>"
                    self.includes.add("<vector>")
                else:
                    # Array of primitives
                    element_type = self._get_cpp_type(value[0] if value else "")
                    class_def['members'][key] = f"std::vector<{element_type}>"
                    self.includes.add("<vector>")
                    
            else:
                # Primitive type
                cpp_type = self._get_cpp_type(value)
                class_def['members'][key] = cpp_type
                if cpp_type == "std::string":
                    self.includes.add("<string>")
        
        self.classes[class_name] = class_def
        return class_def
    
    def _to_class_name(self, name: str) -> str:
        """Convert JSON key to C++ class name"""
        return ''.join(word.capitalize() for word in name.replace('_', ' ').split())
    
    def _get_cpp_type(self, value: Any) -> str:
        """Get C++ type for JSON value"""
        if isinstance(value, bool):
            return "bool"
        elif isinstance(value, int):
            return "int64_t"
        elif isinstance(value, float):
            return "double"
        elif isinstance(value, str):
            return "std::string"
        else:
            return "std::string"  # fallback
    
    def generate_header(self, namespace: str = "") -> str:
        """Generate C++ header file"""
        header = []
        header.append("#pragma once")
        header.append("")
        
        # Includes
        for include in sorted(self.includes):
            header.append(f"#include {include}")
        header.append("#include <json/json.h>")
        header.append("")
        
        if namespace:
            header.append(f"namespace {namespace} {{")
            header.append("")
        
        # Forward declarations
        header.append("// Forward declarations")
        for class_name in self.classes:
            header.append(f"class {class_name};")
        header.append("")
        
        # Base class
        header.append("class JsonSerializable {")
        header.append("public:")
        header.append("    virtual Json::Value toJson() const = 0;")
        header.append("    virtual void fromJson(const Json::Value& json) = 0;")
        header.append("    virtual ~JsonSerializable() = default;")
        header.append("};")
        header.append("")
        
        # Generate classes
        for class_name, class_def in self.classes.items():
            header.extend(self._generate_class(class_def))
            header.append("")
        
        if namespace:
            header.append(f"}} // namespace {namespace}")
        
        return "\n".join(header)
    
    def _generate_class(self, class_def: Dict) -> list:
        """Generate a single C++ class"""
        lines = []
        class_name = class_def['name']
        
        lines.append(f"class {class_name} : public JsonSerializable {{")
        lines.append("public:")
        
        # Members
        for member_name, member_type in class_def['members'].items():
            lines.append(f"    {member_type} {member_name};")
        lines.append("")
        
        # toJson method
        lines.append("    Json::Value toJson() const override {")
        lines.append("        Json::Value json;")
        for member_name, member_type in class_def['members'].items():
            if "std::vector" in member_type:
                lines.append(f"        Json::Value {member_name}Array(Json::arrayValue);")
                lines.append(f"        for (const auto& item : {member_name}) {{")
                if "std::string" in member_type or "bool" in member_type or "int" in member_type or "double" in member_type:
                    lines.append(f"            {member_name}Array.append(item);")
                else:
                    lines.append(f"            {member_name}Array.append(item.toJson());")
                lines.append("        }")
                lines.append(f"        json[\"{member_name}\"] = {member_name}Array;")
            elif member_type in ["std::string", "bool", "int64_t", "double"]:
                lines.append(f"        json[\"{member_name}\"] = {member_name};")
            else:
                lines.append(f"        json[\"{member_name}\"] = {member_name}.toJson();")
        lines.append("        return json;")
        lines.append("    }")
        lines.append("")
        
        # fromJson method
        lines.append("    void fromJson(const Json::Value& json) override {")
        for member_name, member_type in class_def['members'].items():
            lines.append(f"        if (json.isMember(\"{member_name}\")) {{")
            if "std::vector" in member_type:
                lines.append(f"            {member_name}.clear();")
                lines.append(f"            if (json[\"{member_name}\"].isArray()) {{")
                lines.append(f"                for (const auto& item : json[\"{member_name}\"]) {{")
                if "std::string" in member_type:
                    lines.append(f"                    {member_name}.push_back(item.asString());")
                elif "bool" in member_type:
                    lines.append(f"                    {member_name}.push_back(item.asBool());")
                elif "int64_t" in member_type:
                    lines.append(f"                    {member_name}.push_back(item.asInt64());")
                elif "double" in member_type:
                    lines.append(f"                    {member_name}.push_back(item.asDouble());")
                else:
                    # Custom class
                    item_type = member_type.replace("std::vector<", "").replace(">", "")
                    lines.append(f"                    {item_type} obj;")
                    lines.append(f"                    obj.fromJson(item);")
                    lines.append(f"                    {member_name}.push_back(std::move(obj));")
                lines.append("                }")
                lines.append("            }")
            elif member_type == "std::string":
                lines.append(f"            {member_name} = json[\"{member_name}\"].asString();")
            elif member_type == "bool":
                lines.append(f"            {member_name} = json[\"{member_name}\"].asBool();")
            elif member_type == "int64_t":
                lines.append(f"            {member_name} = json[\"{member_name}\"].asInt64();")
            elif member_type == "double":
                lines.append(f"            {member_name} = json[\"{member_name}\"].asDouble();")
            else:
                lines.append(f"            {member_name}.fromJson(json[\"{member_name}\"]);")
            lines.append("        }")
        lines.append("    }")
        
        lines.append("};")
        return lines

def main():
    if len(sys.argv) != 2:
        print("Usage: python json_to_cpp.py <json_file>")
        sys.exit(1)
    
    json_file = sys.argv[1]
    
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    generator = CppClassGenerator()
    generator.analyze_json(data, "GridLabDData")
    
    header_content = generator.generate_header("gridlabd")
    
    # Write to header file
    output_file = json_file.replace('.json', '_classes.h')
    with open(output_file, 'w') as f:
        f.write(header_content)
    
    print(f"Generated C++ classes in {output_file}")

if __name__ == "__main__":
    main()