#!/usr/bin/env python3
import os

def filter_content(input_content):
    return input_content

def delete_file_if_exists(filename, folder="."):
    """
    Delete a file if it exists in the specified folder.
    
    Args:
        filename (str): Name of the file to delete
        folder (str): Path to the folder (default: current directory ".")
    
    Returns:
        bool: True if file was deleted, False if file didn't exist
        
    Raises:
        PermissionError: If you don't have permission to delete the file
        OSError: If there's an OS-related error during deletion
    """
    file_path = os.path.join(folder, filename)
    
    if os.path.exists(file_path) and os.path.isfile(file_path):
        try:
            os.remove(file_path)
            print(f"File '{filename}' deleted successfully from '{folder}'")
            return True
        except (PermissionError, OSError) as e:
            print(f"Error deleting file '{filename}': {e}")
            raise
    else:
        print(f"File '{filename}' not found in '{folder}'")
        return False

def combine_markdown_files(folder_path=".", output_filename="foo.md"):
    """
    Reads all .md files in a folder in alphabetical order and prints their contents.
    
    Args:
        folder_path (str): Path to the folder containing .md files (default is current directory)
    """ 
    # Delete output file, if it exists.  
    delete_file_if_exists(output_filename, folder_path)

    try:
        # Get all files in the directory.
        all_files = os.listdir(folder_path)
        
        # Filter for .md files only.
        md_files = [f for f in all_files if f.lower().endswith('.md')]
        
        if not md_files:
            print("No .md files found in the specified folder.")
            return
        
        # Sort files alphabetically.
        md_files.sort()
        
        print(f"Found {len(md_files)} .md file(s). Traversing in alphabetical order...")
        
        for filename in md_files:
            file_path = os.path.join(folder_path, filename)
            output_path = os.path.join(folder_path, output_filename)
            print(f"📄 FILE: {filename}")
            
            try:
                with open(file_path, 'r', encoding='utf-8') as file_in:
                    with open(output_path, 'a', encoding='utf-8') as file_out:
                        content = file_in.read()
                        if not content.strip():  # Check if file has content
                            print("(This file is empty)")
                        else:
                            file_out.write(filter_content(content))
            except Exception as e:
                print(f"Error reading {filename}: {e}")
            
    except FileNotFoundError:
        print(f"Error: Folder '{folder_path}' does not exist.")
    except PermissionError:
        print(f"Error: Permission denied to access folder '{folder_path}'.")
    except Exception as e:
        print(f"Error accessing folder: {e}")

def main():
    folder_path = '.'
    output_filename = 'foo.md'
    
    combine_markdown_files(folder_path, output_filename)

    return

if __name__ == "__main__":
    main()