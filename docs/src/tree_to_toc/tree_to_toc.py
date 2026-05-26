"""
Note: this script was produced by the AI Incubator, when asked if it could write a script to create a file tree and turn it into a nav: list compatible with mkdocs.yml

"""

import os


def extract_title_from_md(file_path):
    """
    Extract the first markdown header (# Heading) from a markdown file.
    If no header is found, default to using the filename.

    Args:
        file_path (str): Path to the markdown file.

    Returns:
        str: Extracted title or fallback filename without extension.
    """
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line.startswith("# "):  # Identify the first top-level heading (# Heading 1)
                    return line[2:].strip()  # Remove '# ' prefix and return title
        # Fallback to the filename if no title is found
        return os.path.splitext(os.path.basename(file_path))[0]
    except Exception as e:
        print(f"Error processing file {file_path}: {e}")
        return os.path.splitext(os.path.basename(file_path))[0]


def generate_nav_list(root_dir):
    """
    Generate the `nav:` list for MkDocs based on a given directory structure.

    Args:
        root_dir (str): Path to the root directory (e.g., "docs/")

    Returns:
        str: A YAML-formatted `nav:` section for mkdocs.yml
    """
    def traverse_directory(directory):
        """
        Recursively traverse the directory and build a nested dictionary of files and subdirectories.
        """
        nav_dict = {}
        for item in sorted(os.listdir(directory)):  # Sort items for consistent output
            item_path = os.path.join(directory, item)

            if os.path.isdir(item_path):  # If the item is a subdirectory
                nav_dict[item] = traverse_directory(item_path)
            elif item.endswith(".md"):  # If the item is a markdown file
                page_title = extract_title_from_md(item_path)  # Get title or filename
                relative_path = os.path.relpath(item_path, root_dir)  # Get relative path
                nav_dict[page_title] = f"docs/{relative_path.replace(os.sep, '/')}"  # Prepend "docs/" and standardize path separators
        return nav_dict

    def format_nav(nav_dict, indent=0):
        """
        Format the nested dictionary into a YAML-like string for MkDocs.
        """
        nav_list = ""
        for key, value in nav_dict.items():
            if isinstance(value, dict):  # Subdirectory
                nav_list += "  " * indent + f"- {key}:\n"
                nav_list += format_nav(value, indent + 1)
            else:  # Markdown file
                nav_list += "  " * indent + f"- {key}: {value}\n"
        return nav_list

    # Generate the navigation dictionary
    nav_dict = traverse_directory(root_dir)
    
    # Convert the dictionary to YAML-formatted nav list
    return format_nav(nav_dict)


# Path to the `docs/` directory
docs_directory = "../../docs"  # Update this path to your actual docs directory

# Generate and print the nav list
nav_list = generate_nav_list(docs_directory)
print("nav:")
print(nav_list)

print('-------------------------------------------------------------------')
print('Docs will not build with empty pages! Remove placeholders from TOC.')
print('-------------------------------------------------------------------')

# Save the output to `tree.txt`
with open("tree.txt", "w", encoding="utf-8") as txt_file:
    txt_file.write(nav_list)