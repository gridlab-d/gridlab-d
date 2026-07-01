# Sync Check

The Sync Check object implements a synchronization check functionality for switches to close when two grids are within parameters.
It must be a child of a switch and will use the voltage and frequency measurements on the from and to nodes to determine if the switch can be closed.

### Sample

object sync_check {
	name sync_check_obj;
	parent switch3_3B;
	armed false;
	frequency_tolerance 0.01 Hz;
	voltage_tolerance_pu 0.02;
	metrics_period 5.1 ms;	//Rounded up a little to prevent double conversion issues
}

### Sync Check Parameters

#### Properties

**sync_check** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: sync_check table 1 { #tbl:48-sync-check-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **armed** | bool | N/A | O | Flag to arm the synchronization close |
| **frequency_tolerance** | double | Hz | O | tolerance for checking the frequency metric |
| **voltage_tolerance_pu** | double | pu | O | voltage_tolerance in per-unit - used in MAG_DIFF mode |
| **voltage_tolerance** | double | V | O | voltage_tolerance in Volts - used in MAG_DIFF mode - prioritized over voltage_tolerance_pu |
| **metrics_period** | double | s | O | period when both metrics are satisfied |
| **volt_compare_mode** | enumeration | N/A | O | Determines which voltage difference calculation approach is used Valid values: `MAG_DIFF`, `SEP_DIFF`. |
| **voltage_magnitude_tolerance_pu** | double | pu | O | tolerance in per-unit for the difference in voltage magnitudes - used in SEP_DIFF mode |
| **voltage_magnitude_tolerance** | double | V | O | tolerance in Volts for the difference in voltage magnitudes - used in SEP_DIFF mode - prioritized over voltage_magnitude_tolerance_pu |
| **voltage_angle_tolerance** | double | deg | O | tolerance in degrees for the difference in voltage angles - used in SEP_DIFF mode |
| **delta_trigger_mult** | double | N/A | O | multiplier against voltage and frequency tolerances to trigger/maintain deltamode |

??? note "Internal Properties"

	#### Internal Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

	Table: sync_check table 2 { #tbl:48-sync-check-2 }

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| freq_diff_noabs_hz | double | N/A | O | Measurement property: frequency difference in Hz without abs() |
	| volt_A_mag_diff_noabs_pu | double | N/A | O | Measurement property: Difference of phase A voltage magnitude in pu without abs() |
	| volt_B_mag_diff_noabs_pu | double | N/A | O | Measurement property: Difference of phase B voltage magnitude in pu without abs() |
	| volt_C_mag_diff_noabs_pu | double | N/A | O | Measurement property: Difference of phase C voltage magnitude in pu without abs() |
	| freq_diff_hz | double | N/A | O | Measurement property: frequency difference in Hz |
	| volt_A_diff | double | N/A | O | Measurement property: Magnitude of phase A voltage phasor difference in volt |
	| volt_B_diff | double | N/A | O | Measurement property: Magnitude of phase B voltage phasor difference in volt |
	| volt_C_diff | double | N/A | O | Measurement property: Magnitude of phase C voltage phasor difference in volt |
	| volt_A_diff_pu | double | N/A | O | Measurement property: Magnitude of phase A voltage phasor difference in pu |
	| volt_B_diff_pu | double | N/A | O | Measurement property: Magnitude of phase B voltage phasor difference in pu |
	| volt_C_diff_pu | double | N/A | O | Measurement property: Magnitude of phase C voltage phasor difference in pu |
	| volt_A_mag_diff | double | N/A | O | Measurement property: Difference of phase A voltage magnitude in volt |
	| volt_B_mag_diff | double | N/A | O | Measurement property: Difference of phase B voltage magnitude in volt |
	| volt_C_mag_diff | double | N/A | O | Measurement property: Difference of phase C voltage magnitude in volt |
	| volt_A_mag_diff_pu | double | N/A | O | Measurement property: Difference of phase A voltage magnitude in pu |
	| volt_B_mag_diff_pu | double | N/A | O | Measurement property: Difference of phase B voltage magnitude in pu |
	| volt_C_mag_diff_pu | double | N/A | O | Measurement property: Difference of phase C voltage magnitude in pu |
	| volt_A_ang_deg_diff | double | N/A | O | Measurement property: Difference of phase A voltage angle in degree |
	| volt_B_ang_deg_diff | double | N/A | O | Measurement property: Difference of phase B voltage angle in degree |
	| volt_C_ang_deg_diff | double | N/A | O | Measurement property: Difference of phase C voltage angle in degree |
	| nominal_volt_v | double | N/A | O | Measurement Property: Nominal voltage of from/to node of the parent switch |
