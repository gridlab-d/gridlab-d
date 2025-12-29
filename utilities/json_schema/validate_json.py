import os
import json
from jsonschema import Draft7Validator, SchemaError

# Function to load a JSON file from the specified directory
def load_json(file_name, directory=""):
    """
    Loads a JSON file from the specified directory.

    :param file_name: Name of the JSON file
    :param directory: Directory to load the file from
    :return: Parsed JSON data
    """
    try:
        # Determine the path to the JSON file
        file_path = os.path.join(os.path.dirname(__file__), directory, file_name)
        with open(file_path, "r") as json_file:
            return json.load(json_file)
    except FileNotFoundError:
        print(f"The file '{file_name}' was not found in the directory '{directory}'.")
        return None
    except json.JSONDecodeError as e:
        print(f"Error decoding the file '{file_name}': {e}")
        return None


def validate_json(data, json_schema):
    """
    Validates a JSON object against a JSON schema and lists all errors.

    :param data: JSON data to validate
    :param json_schema: JSON schema to validate against
    :return: True if no errors, False otherwise
    """
    if data is None or json_schema is None:
        print("No JSON data or schema to validate.")
        return False
    
    try:
        # Create a validator instance
        validator = Draft7Validator(json_schema)

        # Collect all errors
        errors = sorted(validator.iter_errors(data), key=lambda e: e.path)
        
        if not errors:
            print("JSON is valid.")
            return True

        # Log and display the errors
        print("JSON validation failed with the following errors:")
        for error in errors:
            # Get location in JSON and schema
            json_path = " > ".join(map(str, error.absolute_path))
            schema_path = " > ".join(map(str, error.schema_path))

            print(f"- Error: {error.message}")
            print(f"  JSON Path: {json_path or '(root)'}")
            print(f"  Schema Path: {schema_path or '(root)'}\n")
        
        return False
    except SchemaError as e:
        print(f"Schema error: {e}")
        return False


if __name__ == "__main__":
    # File names
    json_file_name = "TE_CHALLENGE_values.json"  # The JSON file
    schema_file_name = "default_schema.json"  # The schema file

    # Load the schema
    schema = load_json(schema_file_name, directory="output")

    # Load the JSON data
    json_data = load_json(json_file_name, directory="output")

    # Validate the JSON data against the schema
    validate_json(json_data, schema)
