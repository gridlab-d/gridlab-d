## Load Tracker


### Load Tracker Parameters

#### Properties

**load_tracker** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| target | object | N/A | I | target object to track the load of |
| target_property | char256 | N/A | I | property on the target object representing the load |
| operation | enumeration | N/A | I | operation to perform on complex property types Valid values: `REAL`, `IMAGINARY`, `MAGNITUDE`, `ANGLE`. |
| full_scale | double | N/A | I | magnitude of the load at full load, used for feed-forward control |
| setpoint | double | N/A | I | load setpoint to track to |
| deadband | double | N/A | I | percentage deadband |
| damping | double | N/A | I | load setpoint to track to |
| output | double | N/A | IO | output scaling value |
| feedback | double | N/A | O | the feedback signal, for reference purposes |
