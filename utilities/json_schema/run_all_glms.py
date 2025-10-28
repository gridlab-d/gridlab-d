import os
import shutil
import subprocess
import sys

def find_glm_files(base_dir):
    """Recursively find all .glm files in the given directory."""
    glm_files = []
    for root, _, files in os.walk(base_dir):
        for file in files:
            if file.endswith('.glm'):
                glm_files.append(os.path.join(root, file))
    print(f"Found {len(glm_files)} .glm files.")
    return glm_files

def copy_glm_files_to_folder(glm_files, target_folder):
    """Copy .glm files to the target folder if they're not already there."""
    if not os.path.exists(target_folder):
        os.makedirs(target_folder)
    for glm_file in glm_files:
        target_path = os.path.join(target_folder, os.path.basename(glm_file))
        if not os.path.exists(target_path):  # Only copy if the file doesn't already exist
            shutil.copy(glm_file, target_path)
            print(f"Copied {glm_file} to {target_path}")
    print(f"Copied {len(glm_files)} .glm files to {target_folder}.")

def run_glm_to_json(glm_file, script_path, error_files):
    """Run the glm_to_json.py script for the given .glm file."""
    glm_name = os.path.splitext(os.path.basename(glm_file))[0]  # Pass only the base name
    print(f"Running glm_to_json for: {glm_file}")  # Print the file name before processing
    try:
        subprocess.run(
            [sys.executable, script_path, glm_name],
            cwd=os.path.dirname(script_path),  # Ensure the script runs in the correct directory
            check=True
        )
        print(f"Successfully processed: {glm_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error processing {glm_file}: {e}")
        error_files.append(glm_file)

def main():
    # Top-level directory of the project
    project_dir = r"C:\dev\gridlab-d_fork"
    # Target directory for .glm files
    target_dir = os.path.join(os.getcwd(), 'glmFiles')
    # Path to the glm_to_json.py script
    script_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'glm_to_json.py'))

    # Find all .glm files in the project directory
    glm_files = find_glm_files(project_dir)
    if not glm_files:
        print("No .glm files found.")
        return

    # Copy .glm files to the target directory
    copy_glm_files_to_folder(glm_files, target_dir)

    # Find all .glm files in the target directory
    glm_files_in_target = find_glm_files(target_dir)

    # Track errors and total files processed
    error_files = []
    total_files = len(glm_files_in_target)

    # Process each .glm file
    for glm_file in glm_files_in_target:
        run_glm_to_json(glm_file, script_path, error_files)

    # Report results
    print("\n=== Summary ===")
    print(f"Total files processed: {total_files}")
    if error_files:
        print(f"Files with errors: {len(error_files)}")
        for error_file in error_files:
            print(f" - {error_file}")
    else:
        print("All files processed successfully!")

if __name__ == '__main__':
    main()