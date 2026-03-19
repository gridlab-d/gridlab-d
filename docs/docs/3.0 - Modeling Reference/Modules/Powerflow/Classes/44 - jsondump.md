## Jsondump

!!! warning
    This page was automatically generated and requires review.

### Inheritance

| Parent Class |
| --- |

### Jsondump Parameters

#### Properties

| Property Name | Type | Unit | Input | Updates | Description | Evidence (delete after review) |
| --- | --- | --- | --- | --- | --- | --- |
| group | char32 | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">the group ID to output data for (all objects if empty)</div> | <div style="white-space: normal; overflow-wrap: anywhere;">other writes: dump_system</div> |
| filename_dump_system | char256 | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">the file to dump the current data into</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: init</div> |
| filename_dump_reliability | char256 | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">the file to dump the current data into</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: init</div> |
| runtime | timestamp | N/A |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">the time to check voltage data</div> | <div style="white-space: normal; overflow-wrap: anywhere;">runtime writes: commit</div> |
| runcount | int32 | N/A |  |  | <div style="white-space: normal; overflow-wrap: anywhere;">the number of times the file has been written to</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_REFERENCE; init writes: create</div> |
| write_system_info | bool | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Flag indicating whether system information will be written into JSON file or not</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| write_reliability | bool | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Flag indicating whether reliabililty information will be written into JSON file or not</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| write_per_unit | bool | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Output the quantities as per-unit values</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| system_base | double | VA | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">System base power rating for per-unit calculations</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create, init</div> |
| min_node_voltage | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Per-unit minimum voltage level allowed for nodes</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| max_node_voltage | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Per-unit maximum voltage level allowed for nodes</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
