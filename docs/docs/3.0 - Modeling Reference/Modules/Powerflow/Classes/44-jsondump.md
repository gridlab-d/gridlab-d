## Jsondump

!!! warning
    This page was automatically generated and requires review.

### Jsondump Parameters

#### Properties

**jsondump** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| group | char32 | N/A | I | ⚠️ the group ID to output data for (all objects if empty) |
| filename_dump_system | char256 | N/A | I | ⚠️ the file to dump the current data into |
| filename_dump_reliability | char256 | N/A | I | ⚠️ the file to dump the current data into |
| runtime | timestamp | N/A | O | ⚠️ the time to check voltage data |
| runcount | int32 | N/A | — | ⚠️ the number of times the file has been written to |
| write_system_info | bool | N/A | I | ⚠️ Flag indicating whether system information will be written into JSON file or not |
| write_reliability | bool | N/A | I | ⚠️ Flag indicating whether reliabililty information will be written into JSON file or not |
| write_per_unit | bool | N/A | I | ⚠️ Output the quantities as per-unit values |
| system_base | double | VA | I | ⚠️ System base power rating for per-unit calculations |
| min_node_voltage | double | pu | I | ⚠️ Per-unit minimum voltage level allowed for nodes |
| max_node_voltage | double | pu | I | ⚠️ Per-unit maximum voltage level allowed for nodes |
