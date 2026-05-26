#!/usr/bin/env python3
"""
The following prompt was used on claude-sonnet-4-20250514-v1 to generate the following script

"I need a Python script that will read all the Markdown files in my documentation directory walking the entire project directory, looking for lines with "TODO" in them. For each line with a "TODO", there will be a "-" followed by a string indicating the stage of the "TODO" item, followed by another "-" followed by another string for notes. This TODO line should be captured, including the file and the line number, and parsed, sorting each item based on the stage into a dictionary. The keys of the dictionary are the stage and value is a list. This dictionary needs to be printed out to a Markdown-formatted file where each section heading is the stage string and the contents of each section are a Markdown list with contents from the corresponding list in the dictionary."

A new section was hand-written based on the generated code to grab any "TODO"s that don't fit the tagged format

Minor adjustments were made to change the target directory, the output formatting of the report, and the location of where the output report was written. These edits took place in `generate_markdown_report()` and `main()`.

@author Trevor Hardy
"""


import os
import re
from collections import defaultdict

def find_todos_in_markdown_files(root_dir):
    """
    Walk through directory tree and find all TODO items in Markdown files.
    
    Args:
        root_dir (str): Root directory to start searching from
        
    Returns:
        dict: Dictionary with stages as keys and lists of TODO items as values
    """
    todos_by_stage = defaultdict(list)
    
    # Regular expression to match TODO lines
    # Pattern: TODO - stage - notes
    todo_pattern_tagged = re.compile(r'.*TODO\s*-\s*([^-]+?)\s*-\s*(.+)', re.IGNORECASE)

    # Pattern: TODO
    todo_pattern_general = re.compile(r'.*TODO\s*[:\-\*]*\s*(.+)', re.IGNORECASE)
    
    # Walk through all files in the directory tree
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            # Only process Markdown files
            if file.lower().endswith(('.md', '.markdown')):
                file_path = os.path.join(root, file)
                
                try:
                    with open(file_path, 'r', encoding='utf-8') as f:
                        for line_num, line in enumerate(f, 1):
                            # Check if line contains a tagged TODO
                            match_tagged = todo_pattern_tagged.match(line.strip())
                            match_general = todo_pattern_general.match(line.strip())
                            if match_tagged:
                                stage = match_tagged.group(1).strip()
                                notes = match_tagged.group(2).strip()
                                
                                # Create TODO item with metadata
                                todo_item = {
                                    'file': file_path,
                                    'line_number': line_num,
                                    'stage': stage,
                                    'notes': notes,
                                    'original_line': line.strip()
                                }
                                todos_by_stage[stage].append(todo_item)
                                
                            
                            elif match_general:
                                stage = "UNTAGGED"
                                notes = match_general.group(1).strip()
                                
                                # Create TODO item with metadata
                                todo_item = {
                                    'file': file_path,
                                    'line_number': line_num,
                                    'stage': stage,
                                    'notes': notes,
                                    'original_line': line.strip()
                                }
                                todos_by_stage[stage].append(todo_item)
                                
                except (UnicodeDecodeError, IOError) as e:
                    print(f"Warning: Could not read file {file_path}: {e}")
    
    return dict(todos_by_stage)

def generate_markdown_report(todos_dict, output_file, base_dir):
    """
    Generate a Markdown report from the TODOs dictionary.
    
    Args:
        todos_dict (dict): Dictionary with stages as keys and TODO items as values
        output_file (str): Path to output Markdown file
    """
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# TODO Report\n\n")
        f.write(f"This report contains all TODO items found in Markdown files.\n\n")
        
        # Sort stages alphabetically for consistent output
        sorted_stages = sorted(todos_dict.keys())
        
        for stage in sorted_stages:
            f.write(f"## {stage}\n")
            
            todos = todos_dict[stage]
            
            for todo in todos:
                # Create relative path for cleaner display
                rel_path = os.path.relpath(todo['file'], base_dir)
                f.write(f"- `{rel_path}` - l.{todo['line_number']} - {todo['notes']}\n")
            f.write("\n\n")
        
        # Add summary at the end
        total_todos = sum(len(todos) for todos in todos_dict.values())
        f.write(f"---\n\n")
        f.write(f"**Summary:** {total_todos} TODO items found across {len(todos_dict)} stages.\n")

def main():
    """Main function to orchestrate the TODO extraction and report generation."""
    
    # Configuration - modify these paths as needed
    documentation_dir = "../../docs"  # Current directory, change to your docs directory
    output_file = "../../todo_report.md"
    
    print(f"Scanning for TODO items in: {os.path.abspath(documentation_dir)}")
    
    # Find all TODOs
    todos = find_todos_in_markdown_files(documentation_dir)
    
    if not todos:
        print("No TODO items found in Markdown files.")
        return
    
    # Generate report (now passing the base directory)
    generate_markdown_report(todos, output_file, documentation_dir)
    
    # Print summary to console
    total_todos = sum(len(todo_list) for todo_list in todos.values())
    print(f"\nFound {total_todos} TODO items across {len(todos)} stages:")
    
    for stage, todo_list in sorted(todos.items()):
        print(f"  - {stage}: {len(todo_list)} items")
    
    print(f"\nMarkdown report generated: {os.path.abspath(output_file)}")

if __name__ == "__main__":
    main()