for filename in test*; do
  # Check if the 5th character is NOT an underscore
  if [[ "${filename:4:1}" != "_" ]]; then
    # Construct the new filename: "test_" + the rest of the old name
    new_filename="test_${filename:4}"
    # Print the command instead of running it
    echo mv "$filename" "$new_filename"
  fi
done

