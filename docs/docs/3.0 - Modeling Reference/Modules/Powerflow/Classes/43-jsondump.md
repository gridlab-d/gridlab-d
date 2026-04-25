## Jsondump

!!! warning
    This page was automatically generated and requires review.

### Jsondump Parameters

#### Properties

**jsondump** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| group | char32 | N/A |  | ✓ | ⚠️ the group ID to output data for (all objects if empty) |
| filename_dump_system | char256 | N/A | ✓ |  | ⚠️ the file to dump the current data into |
| filename_dump_reliability | char256 | N/A | ✓ |  | ⚠️ the file to dump the current data into |
| runtime | timestamp | N/A |  | ✓ | ⚠️ the time to check voltage data |
| runcount | int32 | N/A |  |  | ⚠️ the number of times the file has been written to |
| write_system_info | bool | N/A | ✓ |  | ⚠️ Flag indicating whether system information will be written into JSON file or not |
| write_reliability | bool | N/A | ✓ |  | ⚠️ Flag indicating whether reliabililty information will be written into JSON file or not |
| write_per_unit | bool | N/A | ✓ |  | ⚠️ Output the quantities as per-unit values |
| system_base | double | VA | ✓ |  | ⚠️ System base power rating for per-unit calculations |
| min_node_voltage | double | pu | ✓ |  | ⚠️ Per-unit minimum voltage level allowed for nodes |
| max_node_voltage | double | pu | ✓ |  | ⚠️ Per-unit maximum voltage level allowed for nodes |
