#!/bin/bash

# Script to regenerate glm_classes.json file
# This script uses gridlabd --modattr to extract module attributes for various modules

# Set the gridlabd executable path (relative to project root)
GRIDLABD_PATH="../../build/bin/gridlabd"
# Using TESP environment
#GRIDLABD_PATH="$INSTDIR/bin/gridlabd"

# Set the output file path (relative to this script's location)
OUTPUT_FILE="references/glm_classes.json"

# Check if gridlabd exists
if [ ! -f "$GRIDLABD_PATH" ]; then
    echo "Error: gridlabd not found at $GRIDLABD_PATH"
    echo "Please build GridLAB-D first using the build system"
    exit 1
fi

echo "Regenerating $OUTPUT_FILE..."

# Start the JSON structure
echo "{" > "$OUTPUT_FILE"

# Climate module
echo "\"climate\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr climate >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Commercial module
echo "\"commercial\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr commercial >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Connection module
echo "\"connection\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr connection >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Generators module
echo "\"generators\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr generators >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Market module
echo "\"market\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr market >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Powerflow module
echo "\"powerflow\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr powerflow >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Reliability module
echo "\"reliability\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr reliability >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Residential module
echo "\"residential\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr residential >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Tape module
echo "\"tape\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr tape >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Assert module
echo "\"assert\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr assert >> "$OUTPUT_FILE"
echo "," >> "$OUTPUT_FILE"

# Optimize module (no comma after this one as it's the last)
echo "\"optimize\": " >> "$OUTPUT_FILE"
"$GRIDLABD_PATH" --modattr optimize >> "$OUTPUT_FILE"

# Close the JSON structure
echo "}" >> "$OUTPUT_FILE"


echo "File location: $(pwd)/$OUTPUT_FILE"